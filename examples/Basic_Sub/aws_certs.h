#pragma once

// User: Paste the contents of your aws_root_ca.pem here
const char AWS_ROOT_CA[] = R"EOF(
-----BEGIN CERTIFICATE-----
[PASTE LONG STRING HERE]
-----END CERTIFICATE-----
)EOF";

// User: Paste the contents of your client.crt here
const char CLIENT_CERT[] = R"EOF(
-----BEGIN CERTIFICATE-----
[PASTE LONG STRING HERE]
-----END CERTIFICATE-----
)EOF";

// User: Paste the contents of your client.key here
const char CLIENT_PRIVATE_KEY[] = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
[PASTE LONG STRING HERE]
-----END RSA PRIVATE KEY-----
)EOF";