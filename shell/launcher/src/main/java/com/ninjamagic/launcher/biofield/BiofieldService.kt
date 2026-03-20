package com.ninjamagic.launcher.biofield

import android.bluetooth.*
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.content.Context
import android.util.Log
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import java.util.UUID

/**
 * Biofield Service — connects to BLE heart rate monitors and
 * translates physiological data into biofield state classifications.
 *
 * Supports standard Bluetooth Heart Rate Profile (HRP) devices:
 * - Chest straps (Polar H10, Garmin HRM-Pro)
 * - Arm bands (Polar Verity Sense, Scosche Rhythm24)
 * - Smart rings (Oura Gen 3)
 * - Smart watches (any device exposing HR service)
 *
 * The biofield state drives UI adaptations:
 * - Animation tempo (slower when resting, faster when charged)
 * - Color accent shifts (cool=focused, warm=charged, green=restorative)
 * - Motion intensity (reduced when depleted)
 */
class BiofieldService(private val context: Context) {

    companion object {
        private const val TAG = "BiofieldService"

        // Standard Bluetooth Heart Rate Service UUID
        val HR_SERVICE_UUID: UUID = UUID.fromString("0000180d-0000-1000-8000-00805f9b34fb")
        val HR_MEASUREMENT_UUID: UUID = UUID.fromString("00002a37-0000-1000-8000-00805f9b34fb")
        val CLIENT_CONFIG_UUID: UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")

        // Classification thresholds
        private const val RESTING_HR_LOW = 50
        private const val RESTING_HR_HIGH = 70
        private const val ACTIVE_HR_THRESHOLD = 100
        private const val HIGH_HR_THRESHOLD = 130
        private const val HIGH_HRV_THRESHOLD = 60f
        private const val LOW_HRV_THRESHOLD = 30f
    }

    private val _state = MutableStateFlow(BiofieldSnapshot())
    val state: StateFlow<BiofieldSnapshot> = _state.asStateFlow()

    private var bluetoothGatt: BluetoothGatt? = null
    private var isScanning = false

    // R-R interval buffer for HRV calculation (last 60 intervals)
    private val rrIntervals = ArrayDeque<Int>(60)

    /** Start scanning for BLE heart rate devices. */
    fun startScan() {
        val bluetoothManager = context.getSystemService(Context.BLUETOOTH_SERVICE) as? BluetoothManager
        val adapter = bluetoothManager?.adapter ?: run {
            Log.w(TAG, "Bluetooth not available")
            return
        }

        if (!adapter.isEnabled) {
            Log.w(TAG, "Bluetooth not enabled")
            return
        }

        val scanner = adapter.bluetoothLeScanner ?: return
        isScanning = true
        Log.i(TAG, "Starting BLE scan for heart rate devices")

        try {
            scanner.startScan(scanCallback)
        } catch (e: SecurityException) {
            Log.e(TAG, "BLE scan permission denied", e)
        }
    }

    /** Stop scanning. */
    fun stopScan() {
        if (!isScanning) return
        val bluetoothManager = context.getSystemService(Context.BLUETOOTH_SERVICE) as? BluetoothManager
        val scanner = bluetoothManager?.adapter?.bluetoothLeScanner ?: return
        try {
            scanner.stopScan(scanCallback)
        } catch (e: SecurityException) {
            Log.e(TAG, "BLE stop scan permission denied", e)
        }
        isScanning = false
    }

    /** Disconnect from current device. */
    fun disconnect() {
        bluetoothGatt?.let {
            try {
                it.close()
            } catch (e: SecurityException) {
                Log.e(TAG, "BLE disconnect permission denied", e)
            }
        }
        bluetoothGatt = null
        _state.value = BiofieldSnapshot()
        Log.i(TAG, "Disconnected")
    }

    // ===== BLE Callbacks =====

    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val device = result.device
            val uuids = result.scanRecord?.serviceUuids
            if (uuids?.any { it.uuid == HR_SERVICE_UUID } == true) {
                Log.i(TAG, "Found HR device: ${device.address}")
                stopScan()
                connectDevice(device)
            }
        }
    }

    private fun connectDevice(device: BluetoothDevice) {
        try {
            bluetoothGatt = device.connectGatt(context, false, gattCallback)
            Log.i(TAG, "Connecting to ${device.address}")
        } catch (e: SecurityException) {
            Log.e(TAG, "BLE connect permission denied", e)
        }
    }

    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                Log.i(TAG, "Connected to GATT server")
                try {
                    gatt.discoverServices()
                } catch (e: SecurityException) {
                    Log.e(TAG, "Service discovery permission denied", e)
                }
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                Log.i(TAG, "Disconnected from GATT server")
                _state.value = _state.value.copy(wearableConnected = false)
            }
        }

        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            if (status != BluetoothGatt.GATT_SUCCESS) return

            val hrService = gatt.getService(HR_SERVICE_UUID) ?: return
            val hrChar = hrService.getCharacteristic(HR_MEASUREMENT_UUID) ?: return

            try {
                gatt.setCharacteristicNotification(hrChar, true)
                val descriptor = hrChar.getDescriptor(CLIENT_CONFIG_UUID)
                descriptor?.let {
                    it.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                    gatt.writeDescriptor(it)
                }
            } catch (e: SecurityException) {
                Log.e(TAG, "Notification enable permission denied", e)
            }

            _state.value = _state.value.copy(
                wearableConnected = true,
                wearableType = "BLE Heart Rate Monitor"
            )
            Log.i(TAG, "HR notifications enabled")
        }

        override fun onCharacteristicChanged(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic
        ) {
            if (characteristic.uuid == HR_MEASUREMENT_UUID) {
                processHeartRateMeasurement(characteristic.value)
            }
        }
    }

    // ===== Signal Processing =====

    private fun processHeartRateMeasurement(data: ByteArray) {
        if (data.isEmpty()) return

        val flags = data[0].toInt()
        val is16Bit = (flags and 0x01) != 0
        val hasRR = (flags and 0x10) != 0

        // Parse heart rate
        val heartRate = if (is16Bit && data.size >= 3) {
            (data[1].toInt() and 0xFF) or ((data[2].toInt() and 0xFF) shl 8)
        } else if (data.size >= 2) {
            data[1].toInt() and 0xFF
        } else {
            return
        }

        // Parse R-R intervals (for HRV calculation)
        if (hasRR) {
            var offset = if (is16Bit) 3 else 2
            // Skip energy expended if present
            if ((flags and 0x08) != 0) offset += 2

            while (offset + 1 < data.size) {
                val rr = (data[offset].toInt() and 0xFF) or
                         ((data[offset + 1].toInt() and 0xFF) shl 8)
                // RR interval in 1/1024 seconds, convert to ms
                val rrMs = (rr * 1000) / 1024
                if (rrMs in 300..2000) { // Valid range
                    if (rrIntervals.size >= 60) rrIntervals.removeFirst()
                    rrIntervals.addLast(rrMs)
                }
                offset += 2
            }
        }

        // Calculate HRV (RMSSD method)
        val hrvMs = calculateHRV()

        // Classify biofield state
        val classification = classifyBiofield(heartRate, hrvMs)

        // Calculate coherence (simplified: based on HRV regularity)
        val coherence = if (rrIntervals.size > 5) {
            val mean = rrIntervals.average()
            val variance = rrIntervals.map { (it - mean) * (it - mean) }.average()
            val cv = if (mean > 0) Math.sqrt(variance) / mean else 1.0
            (1.0 - cv.coerceIn(0.0, 1.0)).toFloat()
        } else 0f

        // Update state
        _state.value = BiofieldSnapshot(
            classification = classification,
            heartRateBpm = heartRate,
            hrvMs = hrvMs,
            coherence = coherence,
            fieldStrength = calculateFieldStrength(heartRate, hrvMs, coherence),
            wearableConnected = true,
            wearableType = _state.value.wearableType,
            lastUpdateMs = System.currentTimeMillis(),
        )
    }

    /** Calculate HRV using RMSSD (Root Mean Square of Successive Differences). */
    private fun calculateHRV(): Float? {
        if (rrIntervals.size < 5) return null

        val intervals = rrIntervals.toList()
        var sumSquaredDiffs = 0.0
        var count = 0

        for (i in 1 until intervals.size) {
            val diff = intervals[i] - intervals[i - 1]
            sumSquaredDiffs += diff.toDouble() * diff.toDouble()
            count++
        }

        return if (count > 0) {
            Math.sqrt(sumSquaredDiffs / count).toFloat()
        } else null
    }

    /** Classify biofield state from HR + HRV. */
    private fun classifyBiofield(heartRate: Int, hrvMs: Float?): BiofieldClassification {
        val hrv = hrvMs ?: return when {
            heartRate < RESTING_HR_HIGH -> BiofieldClassification.RESTORATIVE
            heartRate > ACTIVE_HR_THRESHOLD -> BiofieldClassification.CHARGED
            else -> BiofieldClassification.NEUTRAL
        }

        return when {
            // High HRV + moderate HR = focused/flow
            hrv > HIGH_HRV_THRESHOLD && heartRate in RESTING_HR_HIGH..ACTIVE_HR_THRESHOLD ->
                BiofieldClassification.FOCUSED

            // High HR + any HRV = charged/active
            heartRate > HIGH_HR_THRESHOLD ->
                BiofieldClassification.CHARGED

            // Low HR + high HRV = restorative
            heartRate < RESTING_HR_HIGH && hrv > HIGH_HRV_THRESHOLD ->
                BiofieldClassification.RESTORATIVE

            // Low HRV + low HR = depleted
            hrv < LOW_HRV_THRESHOLD && heartRate < RESTING_HR_HIGH ->
                BiofieldClassification.DEPLETED

            // Low HRV + high HR = unsettled/stressed
            hrv < LOW_HRV_THRESHOLD && heartRate > ACTIVE_HR_THRESHOLD ->
                BiofieldClassification.UNSETTLED

            else -> BiofieldClassification.NEUTRAL
        }
    }

    /** Calculate overall field strength (0.0 - 1.0). */
    private fun calculateFieldStrength(heartRate: Int, hrvMs: Float?, coherence: Float): Float {
        val hrScore = when {
            heartRate in RESTING_HR_LOW..RESTING_HR_HIGH -> 0.9f
            heartRate in (RESTING_HR_HIGH + 1)..ACTIVE_HR_THRESHOLD -> 0.7f
            else -> 0.4f
        }
        val hrvScore = when {
            hrvMs == null -> 0.3f
            hrvMs > HIGH_HRV_THRESHOLD -> 0.9f
            hrvMs > LOW_HRV_THRESHOLD -> 0.6f
            else -> 0.3f
        }
        return (hrScore * 0.3f + hrvScore * 0.4f + coherence * 0.3f).coerceIn(0f, 1f)
    }
}
