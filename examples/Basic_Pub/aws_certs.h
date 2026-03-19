#pragma once

// User: Paste the contents of your aws_root_ca.pem here
const char AWS_ROOT_CA[] = R"EOF(
-----BEGIN CERTIFICATE-----
[Paste Long String Here]
-----END CERTIFICATE-----
)EOF";

// User: Paste the contents of your client.crt here
const char CLIENT_CERT[] = R"EOF(
-----BEGIN CERTIFICATE-----
[Paste Long String Here]
-----END CERTIFICATE-----
)EOF";

// User: Paste the contents of your client.key here
const char CLIENT_PRIVATE_KEY[] = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
[Paste Long String Here]
-----END RSA PRIVATE KEY-----
)EOF";