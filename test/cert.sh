#!/bin/bash

BASE_DIR=$PWD/build/test_certs
CA_DIR=$BASE_DIR/ca
CERT_DIR=$BASE_DIR/certs
CRL_DIR=$BASE_DIR/crl

rm "$BASE_DIR" -rf
mkdir -p "$CA_DIR" "$CERT_DIR" "$CRL_DIR"
cd "$BASE_DIR"

set -e

gen_ca() {
  openssl genrsa -out "$CA_DIR/ca.key.pem" 2048
  openssl req -x509 -new -nodes -key "$CA_DIR/ca.key.pem" \
    -sha256 -days 3650 -out "$CA_DIR/ca.pem" \
    -subj "/C=CN/ST=Test/L=Test/O=UT_CA/OU=Dev/CN=Test Root CA"
}

gen_cert1() {
  openssl genrsa -out "$CERT_DIR/cert1.key.pem" 2048
  openssl req -new -key "$CERT_DIR/cert1.key.pem" -out "$CERT_DIR/cert1.csr" \
    -subj "/C=CN/ST=Test/L=Test/O=UT/OU=Dev/CN=Cert1"
  openssl x509 -req -in "$CERT_DIR/cert1.csr" -CA "$CA_DIR/ca.pem" -CAkey "$CA_DIR/ca.key.pem" \
    -CAcreateserial -out "$CERT_DIR/cert1.pem" -days 7 -sha256
  rm "$CERT_DIR/cert1.csr"
}

gen_cert2() {
  openssl genrsa -out "$CERT_DIR/cert2.key.pem" 2048
  openssl req -new -key "$CERT_DIR/cert2.key.pem" -out "$CERT_DIR/cert2.csr" \
    -subj "/C=CN/ST=Test/L=Test/O=UT/OU=Dev/CN=Cert2"
  openssl x509 -req -in "$CERT_DIR/cert2.csr" -CA "$CA_DIR/ca.pem" -CAkey "$CA_DIR/ca.key.pem" \
    -CAcreateserial -out "$CERT_DIR/cert2.pem" -days 365 -sha256
  rm "$CERT_DIR/cert2.csr"
}

gen_cert3() {
  openssl genrsa -out "$CERT_DIR/cert3.key.pem" 2048
  openssl req -new -key "$CERT_DIR/cert3.key.pem" -out "$CERT_DIR/cert3.csr" \
    -subj "/C=CN/ST=Test/L=Test/O=UT/OU=Dev/CN=Cert3"
  openssl x509 -req -in "$CERT_DIR/cert3.csr" -CA "$CA_DIR/ca.pem" -CAkey "$CA_DIR/ca.key.pem" \
    -CAcreateserial -out "$CERT_DIR/cert3.pem" -days -1 -sha256
  rm "$CERT_DIR/cert3.csr"
}

gen_crl_conf() {
cat > "$CRL_DIR/openssl.cnf" <<EOF
[ ca ]
default_ca = CA_default

[ CA_default ]
dir             = $CRL_DIR
database        = \$dir/index.txt
new_certs_dir   = \$dir
certificate     = $CA_DIR/ca.pem
serial          = \$dir/serial
crlnumber       = \$dir/crlnumber
crl             = \$dir/ca.crl.pem
private_key     = $CA_DIR/ca.key.pem
default_md      = sha256
default_crl_days= 3650
policy          = policy_any

[ policy_any ]
commonName              = supplied
EOF
}

gen_crl() {
  touch "$CRL_DIR/index.txt"
  echo 1000 > "$CRL_DIR/serial"
  echo 1000 > "$CRL_DIR/crlnumber"

  openssl ca -config "$CRL_DIR/openssl.cnf" -revoke "$CERT_DIR/cert2.pem" \
    -keyfile "$CA_DIR/ca.key.pem" -cert "$CA_DIR/ca.pem" -batch

  openssl ca -gencrl -config "$CRL_DIR/openssl.cnf" -out "$CRL_DIR/ca.crl.pem" -batch
}

gen_ca
gen_cert1
gen_cert2
gen_cert3
gen_crl_conf
gen_crl

echo "Done. Generated files in: $BASE_DIR"
echo "  CA:     $CA_DIR/ca.pem"
echo "  cert1:  $CERT_DIR/cert1.pem (7 days)"
echo "  cert2:  $CERT_DIR/cert2.pem (revoked)"
echo "  cert3:  $CERT_DIR/cert3.pem (expired)"
echo "  CRL:    $CRL_DIR/ca.crl.pem"
