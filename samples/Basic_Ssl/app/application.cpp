/**
 * Please, note, that in order to run this sample you should recompile Sming with ENABLE_SSL=1.
 * The following three commands should be enough:
 *
 * cd Sming/Sming
 * make clean
 * make ENABLE_SSL=1
 */

#include <user_config.h>
#include <SmingCore/SmingCore.h>

// If you want, you can define WiFi settings globally in Eclipse Environment Variables
#ifndef WIFI_SSID
	#define WIFI_SSID "PleaseEnterSSID" // Put you SSID and Password here
	#define WIFI_PWD "PleaseEnterPass"
#endif

Timer procTimer;
HttpClient downloadClient;


/* Debug SSL functions */
void displaySessionId(SSL *ssl)
{
    int i;
    const uint8_t *session_id = ssl_get_session_id(ssl);
    int sess_id_size = ssl_get_session_id_size(ssl);

    if (sess_id_size > 0)
    {
        debugf("-----BEGIN SSL SESSION PARAMETERS-----");
        for (i = 0; i < sess_id_size; i++)
        {
        	m_printf("%02x", session_id[i]);
        }

        debugf("\n-----END SSL SESSION PARAMETERS-----");
    }
}

/**
 * Display what cipher we are using
 */
void displayCipher(SSL *ssl)
{
	m_printf("CIPHER is ");
    switch (ssl_get_cipher_id(ssl))
    {
        case SSL_AES128_SHA:
        	m_printf("AES128-SHA");
            break;

        case SSL_AES256_SHA:
        	m_printf("AES256-SHA");
            break;

        case SSL_RC4_128_SHA:
        	m_printf("RC4-SHA");
            break;

        case SSL_RC4_128_MD5:
        	m_printf("RC4-MD5");
            break;

        default:
        	m_printf("Unknown - %d", ssl_get_cipher_id(ssl));
            break;
    }

    m_printf("\n");
}

void onDownload(HttpClient& client, bool success)
{
	debugf("Got response code: %d", client.getResponseCode());
	debugf("Got content starting with: %s", client.getResponseString().substring(0, 50).c_str());
	debugf("Success: %d", success);

	SSL* ssl = downloadClient.getSsl();
	if (ssl) {
		const char *common_name = ssl_get_cert_dn(ssl,SSL_X509_CERT_COMMON_NAME);
		if (common_name) {
			debugf("Common Name:\t\t\t%s\n", common_name);
		}
		displayCipher(ssl);
		displaySessionId(ssl);
	}
}

void connectOk()
{
	const uint8_t googleSha1Fingerprint[] = { 0xB1, 0x4C, 0x9E, 0xE1, 0xD5, 0x10, 0xD3, 0xA1, 0x73, 0x15,
											  0xDF, 0xC4, 0x2D, 0xDA, 0x25, 0x7C, 0xD3, 0x95, 0xF6, 0x37 };

	debugf("Connected. Got IP: %s", WifiStation.getIP().toString().c_str());
	downloadClient.addSslOptions(SSL_SERVER_VERIFY_LATER);
	/*
	 * The line below shows how to trust only a certificate that matches the SHA1 fingerprint.
	 * When google changes their certificate the SHA1 fingerprint should not match any longer.
	 */
	downloadClient.setSslFingerprint(googleSha1Fingerprint, 20);
	downloadClient.downloadString("https://www.google.com/", onDownload);
}

void connectFail()
{
	debugf("I'm NOT CONNECTED!");
	WifiStation.waitConnection(connectOk, 10, connectFail); // Repeat and check again
}

void init()
{
	Serial.begin(SERIAL_BAUD_RATE);
	Serial.systemDebugOutput(true); // Allow debug print to serial
	Serial.println("Ready for SSL tests");

	// Setup the WIFI connection
	WifiStation.enable(true);
	WifiStation.config(WIFI_SSID, WIFI_PWD); // Put you SSID and Password here

	// Run our method when station was connected to AP (or not connected)
	WifiStation.waitConnection(connectOk, 30, connectFail); // We recommend 20+ seconds at start
}
