#!/usr/bin/env python3

import argparse
import base64
import codecs
import json
import sys
import time
from http.server import BaseHTTPRequestHandler, HTTPServer

import requests
from Crypto.Cipher import AES


def decrypt(ciphertext, key):
    cipher = AES.new(codecs.decode(key, 'hex_codec'), AES.MODE_ECB)
    result = cipher.decrypt(base64.b64decode(ciphertext))
    result = result.rstrip(b'\x00').decode('utf-8')
    return result


def encrypt(plaintext, key):
    plaintext = plaintext.encode('utf-8')
    plaintext = plaintext + \
                (AES.block_size - len(plaintext) % AES.block_size) * b'\x00'
    cipher = AES.new(codecs.decode(key, 'hex_codec'), AES.MODE_ECB)
    result = cipher.encrypt(plaintext)
    result = base64.b64encode(result)
    return result.decode('utf-8')


def test_encryption():
    plaintext = '{"id":"/gateway/DateTime","type":"stringValue",' \
                '"writeable":1,"recordable":0,"value":"2017-07-21T17:15:00"}'
    key = '11111111111111111111111111111111' \
          '11111111111111111111111111111111'
    ciphertext = 'lSNasjLWs9oUC0hkNB0AcMDV+jQobTQD7KwHjDDfv8GeDKNR+TiFG8wFf' \
                 'DAiCjlnSUqLxNa+RWPF9upIJzQ01M6vtBpJWUUvGfG8WH8fJRt0WKiMbj' \
                 '914aS7pEdWQGuC1+WGKVvCFAN+Nz2bAezNpA=='
    encrypted = encrypt(plaintext, key)
    assert encrypted == ciphertext
    decrypted = decrypt(ciphertext, key)
    assert decrypted == plaintext


class BuderusServer(BaseHTTPRequestHandler):
    CONTENT = {}
    KEY = ''

    def do_GET(self):
        if self.path in self.CONTENT:
            self.send_response(200)
            self.send_header("Content-type", "application/json")
            self.end_headers()
            data = json.dumps(self.CONTENT[self.path])
            self.wfile.write(encrypt(data, self.KEY).encode('utf-8'))
        else:
            self.send_error(404, 'file not found')


def get_value(url, settings):
    headers = {'User-Agent': 'TeleHeater/2.2.3', 'Accept': 'application/json'}
    print("Requesting {}".format(url))
    try:
        response = requests.get(
            'http://{}{}'.format(settings.host, url), headers=headers)
        if response.status_code == 200:
            try:
                data = json.loads(decrypt(response.text, settings.key))
                return data
            except Exception as err:
                print("Failed to parse: {}: {}".format(url, err))
                return None
        else:
            print("Failed to get value: {}".format(url))
            return None
    except requests.RequestException as err:
        raise IOError


def get_values_recursive(url, settings, result):
    data = get_value(url, settings)
    if data:
        result[url] = data
        if 'references' in data:
            for key in data['references']:
                time.sleep(0.1)
                get_values_recursive(key['id'], settings, result)


def query(settings):
    urls = ['/system', '/gateway', '/heatSources', '/heatingCircuits',
            '/dhwCircuits', '/solarCircuits', '/recordings', '/notifications']
    if settings.args:
        urls = settings.args
    result = {}
    print("Please wait, this might take a bit...")
    try:
        for url in urls:
            get_values_recursive(url, settings, result)
    except KeyboardInterrupt:
        print("Interrupted")
    except IOError:
        print("Connection error")

    json_settings = {'sort_keys': True, 'indent': 4, 'separators': (',', ': ')}
    if settings.file:
        with open('out.json', 'w') as outfile:
            json.dump(result, outfile, **json_settings)
    else:
        print(json.dumps(result, **json_settings))


def emulate(settings):
    data = {}
    with open(settings.file) as data_file:
        data = json.load(data_file)
    server = HTTPServer(settings.host, BuderusServer)
    BuderusServer.CONTENT = data
    BuderusServer.KEY = settings.key
    print("Server listening on {}:{} ...".format(
        settings.host[0], settings.host[1]))
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    server.server_close()


def main():
    parser = argparse.ArgumentParser(description='Buderus KM200 Utility')
    parser.add_argument('--mode', choices=['query', 'emulate'],
                        default='query',
                        help="query: connect to real Buderus KM200 Gateway; "
                             "emulate: emulate a Buderus KM200 Gateway")
    parser.add_argument('host',
                        help="query: connect to host; emulate: bind to host")
    parser.add_argument('key',
                        help="en/decryption key")
    parser.add_argument('file', nargs='?',
                        help="query: write output to file (default: stdout); "
                             "emulate: read input from file")
    parser.add_argument('--args', nargs=argparse.REMAINDER)
    settings = parser.parse_args()
    print(settings)

    if len(settings.key) != 64:
        parser.error("key must be 64 hex characters (256 bit)")
    try:
        int(settings.key, 16)
    except ValueError:
        parser.error("key must be 64 hex characters (256 bit)")
    if settings.mode == 'query':
        if not settings.host:
            parser.error("query mode requires host parameter")
        query(settings)
    elif settings.mode == 'emulate':
        if ':' in settings.host:
            host = settings.host.rsplit(':')
            settings.host = (host[0], int(host[1]))
        else:
            settings.host = (settings.host, int(5000))
        if not settings.file:
            parser.error("emulate mode requires file parameter")
        emulate(settings)


if __name__ == '__main__':
    main()
