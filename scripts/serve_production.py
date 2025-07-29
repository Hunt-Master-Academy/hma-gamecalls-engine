#!/usr/bin/env python3
# serve_production.py - Production-ready server with security headers

from http.server import HTTPServer, SimpleHTTPRequestHandler
import sys
import os
import ssl
import gzip
import mimetypes
from pathlib import Path

class ProductionRequestHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        # Initialize MIME types
        mimetypes.add_type('application/wasm', '.wasm')
        mimetypes.add_type('application/javascript', '.js')
        super().__init__(*args, **kwargs)

    def end_headers(self):
        # Security headers for production
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('X-Content-Type-Options', 'nosniff')
        self.send_header('X-Frame-Options', 'DENY')
        self.send_header('X-XSS-Protection', '1; mode=block')
        self.send_header('Strict-Transport-Security', 'max-age=31536000; includeSubDomains')

        # Cache control
        if self.path.endswith(('.wasm', '.js')):
            self.send_header('Cache-Control', 'public, max-age=3600')  # 1 hour
        elif self.path.endswith(('.html', '.htm')):
            self.send_header('Cache-Control', 'no-cache, must-revalidate')
        else:
            self.send_header('Cache-Control', 'public, max-age=86400')  # 1 day

        super().end_headers()

    def do_GET(self):
        # Handle WASM files with proper MIME type
        if self.path.endswith('.wasm'):
            self.send_response(200)
            self.send_header('Content-Type', 'application/wasm')
            self.send_header('Content-Encoding', 'identity')
            self.end_headers()

            file_path = self.translate_path(self.path)
            try:
                with open(file_path, 'rb') as f:
                    self.wfile.write(f.read())
            except FileNotFoundError:
                self.send_error(404, "File not found")
            return

        # Default handling for other files
        super().do_GET()

def run_server(port=8080, directory=None, use_ssl=False, cert_file=None, key_file=None):
    """Run the production server"""

    if directory:
        os.chdir(directory)

    server_address = ('', port)
    httpd = HTTPServer(server_address, ProductionRequestHandler)

    if use_ssl and cert_file and key_file:
        # Setup SSL context
        context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
        context.load_cert_chain(cert_file, key_file)
        httpd.socket = context.wrap_socket(httpd.socket, server_side=True)
        protocol = "HTTPS"
    else:
        protocol = "HTTP"

    print(f"Starting {protocol} server on port {port}")
    print(f"Serving directory: {os.getcwd()}")
    print(f"Access at: {protocol.lower()}://localhost:{port}")

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down server...")
        httpd.shutdown()

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description='Production server for Huntmaster Engine')
    parser.add_argument('--port', '-p', type=int, default=8080,
                       help='Port to serve on (default: 8080)')
    parser.add_argument('--directory', '-d', type=str, default='dist',
                       help='Directory to serve (default: dist)')
    parser.add_argument('--ssl', action='store_true',
                       help='Enable SSL/HTTPS')
    parser.add_argument('--cert', type=str,
                       help='SSL certificate file')
    parser.add_argument('--key', type=str,
                       help='SSL private key file')

    args = parser.parse_args()

    if args.ssl and (not args.cert or not args.key):
        print("Error: SSL requires both --cert and --key arguments")
        sys.exit(1)

    run_server(args.port, args.directory, args.ssl, args.cert, args.key)
