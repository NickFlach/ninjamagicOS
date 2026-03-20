package com.ninjamagic.launcher.spacechild

import android.util.Log
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URL

/**
 * Space Child Auth client for ninjamagicOS.
 * Authenticates with the central Space-Child-Dream auth hub
 * to sync user profile, preferences, and artifacts across
 * the Space Child ecosystem.
 *
 * Auth modes:
 * - Direct: email/password login via API
 * - SSO: redirect to Space-Child-Dream for OAuth-style login
 *
 * Subdomain: phone.spacechild.love
 */
class SpaceChildAuth {

    companion object {
        private const val TAG = "SpaceChildAuth"
        private const val AUTH_BASE_URL = "https://spacechild.love/api/space-child-auth"
        private const val APP_SUBDOMAIN = "ninjamagic-phone"
    }

    data class User(
        val id: String,
        val email: String,
        val displayName: String,
        val avatarUrl: String?,
        val biofieldEnabled: Boolean = false,
        val consciousnessProfile: String? = null,
    )

    data class AuthState(
        val isAuthenticated: Boolean = false,
        val user: User? = null,
        val accessToken: String? = null,
        val refreshToken: String? = null,
        val error: String? = null,
    )

    private val _state = MutableStateFlow(AuthState())
    val state: StateFlow<AuthState> = _state.asStateFlow()

    /** Login with email and password. */
    suspend fun login(email: String, password: String): Result<User> {
        return try {
            val body = JSONObject().apply {
                put("email", email)
                put("password", password)
                put("app", APP_SUBDOMAIN)
            }

            val response = httpPost("$AUTH_BASE_URL/login", body.toString())
            val json = JSONObject(response)

            val user = parseUser(json.getJSONObject("user"))
            val accessToken = json.getString("accessToken")
            val refreshToken = json.getString("refreshToken")

            _state.value = AuthState(
                isAuthenticated = true,
                user = user,
                accessToken = accessToken,
                refreshToken = refreshToken,
            )

            Log.i(TAG, "Logged in: ${user.displayName}")
            Result.success(user)
        } catch (e: Exception) {
            Log.e(TAG, "Login failed", e)
            _state.value = _state.value.copy(error = e.message)
            Result.failure(e)
        }
    }

    /** Register a new account. */
    suspend fun register(email: String, password: String, displayName: String): Result<User> {
        return try {
            val body = JSONObject().apply {
                put("email", email)
                put("password", password)
                put("displayName", displayName)
                put("app", APP_SUBDOMAIN)
            }

            val response = httpPost("$AUTH_BASE_URL/register", body.toString())
            val json = JSONObject(response)
            val user = parseUser(json.getJSONObject("user"))

            Log.i(TAG, "Registered: ${user.displayName}")
            Result.success(user)
        } catch (e: Exception) {
            Log.e(TAG, "Registration failed", e)
            Result.failure(e)
        }
    }

    /** Refresh the access token using the refresh token. */
    suspend fun refreshAccessToken(): Boolean {
        val refreshToken = _state.value.refreshToken ?: return false

        return try {
            val body = JSONObject().apply {
                put("refreshToken", refreshToken)
            }

            val response = httpPost("$AUTH_BASE_URL/refresh", body.toString())
            val json = JSONObject(response)

            _state.value = _state.value.copy(
                accessToken = json.getString("accessToken"),
                refreshToken = json.optString("refreshToken", refreshToken),
            )

            true
        } catch (e: Exception) {
            Log.e(TAG, "Token refresh failed", e)
            false
        }
    }

    /** Get SSO authorization URL for browser-based login. */
    fun getSsoUrl(): String {
        return "$AUTH_BASE_URL/sso/authorize?app=$APP_SUBDOMAIN" +
               "&redirect_uri=ninjamagic://sso/callback"
    }

    /** Handle SSO callback with tokens from URL. */
    fun handleSsoCallback(accessToken: String, refreshToken: String) {
        _state.value = AuthState(
            isAuthenticated = true,
            accessToken = accessToken,
            refreshToken = refreshToken,
        )
        // Fetch user profile
        Log.i(TAG, "SSO callback received")
    }

    /** Logout and clear tokens. */
    suspend fun logout() {
        val token = _state.value.accessToken
        if (token != null) {
            try {
                httpPost("$AUTH_BASE_URL/logout", "{}", token)
            } catch (e: Exception) {
                Log.w(TAG, "Logout API call failed (non-fatal)", e)
            }
        }
        _state.value = AuthState()
        Log.i(TAG, "Logged out")
    }

    /** Get the current user (fetch from server if needed). */
    suspend fun fetchCurrentUser(): User? {
        val token = _state.value.accessToken ?: return null

        return try {
            val response = httpGet("$AUTH_BASE_URL/user", token)
            val json = JSONObject(response)
            val user = parseUser(json.getJSONObject("user"))

            _state.value = _state.value.copy(user = user)
            user
        } catch (e: Exception) {
            Log.e(TAG, "Fetch user failed", e)
            null
        }
    }

    // ===== Helpers =====

    private fun parseUser(json: JSONObject): User {
        return User(
            id = json.getString("id"),
            email = json.getString("email"),
            displayName = json.optString("displayName", json.getString("email")),
            avatarUrl = json.optString("avatarUrl", null),
            biofieldEnabled = json.optBoolean("biofieldEnabled", false),
            consciousnessProfile = json.optString("consciousnessProfile", null),
        )
    }

    private fun httpPost(url: String, body: String, bearerToken: String? = null): String {
        val conn = URL(url).openConnection() as HttpURLConnection
        conn.requestMethod = "POST"
        conn.setRequestProperty("Content-Type", "application/json")
        bearerToken?.let { conn.setRequestProperty("Authorization", "Bearer $it") }
        conn.doOutput = true
        conn.outputStream.write(body.toByteArray())

        val code = conn.responseCode
        val stream = if (code in 200..299) conn.inputStream else conn.errorStream
        val response = stream.bufferedReader().readText()

        if (code !in 200..299) {
            throw RuntimeException("HTTP $code: $response")
        }
        return response
    }

    private fun httpGet(url: String, bearerToken: String): String {
        val conn = URL(url).openConnection() as HttpURLConnection
        conn.requestMethod = "GET"
        conn.setRequestProperty("Authorization", "Bearer $bearerToken")

        val code = conn.responseCode
        val stream = if (code in 200..299) conn.inputStream else conn.errorStream
        val response = stream.bufferedReader().readText()

        if (code !in 200..299) {
            throw RuntimeException("HTTP $code: $response")
        }
        return response
    }
}
