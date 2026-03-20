//! msi-probe — CLI tool to probe the MSI substrate
//!
//! Usage: msi-probe
//! Connects to /dev/msi and prints substrate capabilities.

fn main() {
    env_logger::init();

    println!("=== ninjamagicOS MSI Substrate Probe ===\n");

    match msi::Substrate::connect() {
        Ok(substrate) => {
            let v = substrate.version();
            let caps = substrate.capabilities();

            println!("MSI Version: {}", v);
            println!();
            println!("=== Hardware Capabilities ===");
            println!("  Lanes:     {}-{} (realtime: {})",
                     caps.lanes_min, caps.lanes_max, caps.lanes_realtime);
            println!("  Topics:    {} max", caps.events_max_topics);
            println!("  State:     {} bytes max", caps.state_max_bytes);
            println!("  Security:  {:?} (attestation: {})",
                     caps.security_model, caps.security_attest);
            println!();
            println!("=== Accelerators ===");
            println!("  CPU: {}", caps.accel_cpu);
            println!("  GPU: {}", caps.accel_gpu);
            println!("  NPU: {}", caps.accel_npu);
            println!("  DSP: {}", caps.accel_dsp);
            println!();

            // Determine device based on capabilities
            if caps.accel_npu && !caps.accel_dsp {
                println!("Detected: Google Tensor GS201 (Pixel 7)");
            } else if caps.accel_dsp && !caps.accel_npu {
                println!("Detected: Qualcomm Snapdragon 695 (Nord N30)");
            } else {
                println!("Detected: Unknown SoC");
            }

            println!();
            println!("Substrate ready. Awaiting cognitive programs.");
        }
        Err(e) => {
            eprintln!("ERROR: {}", e);
            eprintln!();
            eprintln!("Is the MSI kernel module loaded?");
            eprintln!("  Try: insmod /vendor/lib/modules/msi.ko");
            std::process::exit(1);
        }
    }
}
