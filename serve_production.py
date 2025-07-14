#!/usr/bin/env python3
"""
Production Web Server for Huntmaster Engine WASM Testing
Provides proper CORS headers, MIME types, and SSL support for user testing
"""

import http.server
import socketserver
import ssl
import os
import sys
import argparse
import json
from pathlib import Path
from urllib.parse import urlparse, parse_qs

class HuntmasterHandler(http.server.SimpleHTTPRequestHandler):
    """Enhanced request handler with proper WASM support and security headers"""
    
    def __init__(self, *args, **kwargs):
        # Serve from project root
        super().__init__(*args, directory=str(Path(__file__).parent), **kwargs)
    
    def end_headers(self):
        """Add security and CORS headers required for WASM"""
        
        # Essential headers for SharedArrayBuffer and WASM
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        
        # CORS headers for development
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type, Authorization, X-Requested-With')
        self.send_header('Access-Control-Max-Age', '86400')
        
        # Security headers
        self.send_header('X-Content-Type-Options', 'nosniff')
        self.send_header('X-Frame-Options', 'SAMEORIGIN')
        self.send_header('X-XSS-Protection', '1; mode=block')
        
        # Cache control for development
        if self.path.endswith(('.wasm', '.js')):
            self.send_header('Cache-Control', 'no-cache, must-revalidate')
        elif self.path.endswith(('.wav', '.mp3', '.ogg')):
            self.send_header('Cache-Control', 'public, max-age=3600')
        
        super().end_headers()
    
    def do_OPTIONS(self):
        """Handle preflight CORS requests"""
        self.send_response(200)
        self.end_headers()
    
    def guess_type(self, path):
        """Enhanced MIME type detection for web audio applications"""
        mimetype, encoding = super().guess_type(path)
        
        # Override for specific file types
        if path.endswith('.wasm'):
            return 'application/wasm', encoding
        elif path.endswith('.js'):
            return 'application/javascript', encoding
        elif path.endswith('.ts'):
            return 'application/typescript', encoding
        elif path.endswith('.wav'):
            return 'audio/wav', encoding
        elif path.endswith('.mp3'):
            return 'audio/mpeg', encoding
        elif path.endswith('.ogg'):
            return 'audio/ogg', encoding
        elif path.endswith('.json'):
            return 'application/json', encoding
        
        return mimetype, encoding
    
    def do_GET(self):
        """Enhanced GET handler with API endpoints"""
        
        # Parse URL
        parsed = urlparse(self.path)
        
        # API endpoints
        if parsed.path.startswith('/api/'):
            self.handle_api_request(parsed)
            return
        
        # Redirect root to web interface
        if parsed.path == '/':
            self.send_response(302)
            self.send_header('Location', '/web/')
            self.end_headers()
            return
        
        # Default file serving
        super().do_GET()
    
    def handle_api_request(self, parsed_url):
        """Handle API requests for engine status and testing"""
        
        path = parsed_url.path
        query = parse_qs(parsed_url.query)
        
        try:
            if path == '/api/status':
                # Engine status endpoint
                response = {
                    'status': 'ready',
                    'version': '1.0.0',
                    'wasmSupported': True,
                    'masterCalls': self.get_available_master_calls(),
                    'buildInfo': self.get_build_info()
                }
                
            elif path == '/api/master-calls':
                # List available master calls
                response = {
                    'calls': self.get_available_master_calls()
                }
                
            elif path == '/api/test-audio':
                # Get test audio files
                response = {
                    'testFiles': self.get_test_audio_files()
                }
                
            else:
                self.send_error(404, 'API endpoint not found')
                return
            
            # Send JSON response
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps(response, indent=2).encode())
            
        except Exception as e:
            self.send_error(500, f'Internal server error: {str(e)}')
    
    def get_available_master_calls(self):
        """Get list of available master call files"""
        master_calls_dir = Path('data/master_calls')
        if not master_calls_dir.exists():
            return []
        
        calls = []
        for wav_file in master_calls_dir.glob('*.wav'):
            calls.append({
                'name': wav_file.stem,
                'filename': wav_file.name,
                'path': f'/data/master_calls/{wav_file.name}',
                'size': wav_file.stat().st_size
            })
        
        return calls
    
    def get_test_audio_files(self):
        """Get list of test audio files"""
        test_dir = Path('data/test_audio')
        if not test_dir.exists():
            return []
        
        files = []
        for audio_file in test_dir.glob('*.wav'):
            files.append({
                'name': audio_file.stem,
                'filename': audio_file.name,
                'path': f'/data/test_audio/{audio_file.name}',
                'size': audio_file.stat().st_size
            })
        
        return files
    
    def get_build_info(self):
        """Get build information"""
        wasm_file = Path('web/dist/huntmaster-engine.wasm')
        js_file = Path('web/dist/huntmaster-engine.js')
        
        return {
            'wasmExists': wasm_file.exists(),
            'wasmSize': wasm_file.stat().st_size if wasm_file.exists() else 0,
            'jsExists': js_file.exists(),
            'jsSize': js_file.stat().st_size if js_file.exists() else 0,
            'lastBuild': wasm_file.stat().st_mtime if wasm_file.exists() else None
        }

def create_ssl_context(cert_file=None, key_file=None):
    """Create SSL context for HTTPS serving"""
    
    if not cert_file or not key_file:
        # Generate self-signed certificate for development
        print("🔐 Generating self-signed certificate for HTTPS...")
        
        import subprocess
        
        cert_file = 'server.crt'
        key_file = 'server.key'
        
        # Generate private key and certificate
        subprocess.run([
            'openssl', 'req', '-x509', '-newkey', 'rsa:4096', '-nodes',
            '-out', cert_file, '-keyout', key_file, '-days', '365',
            '-subj', '/CN=localhost'
        ], check=True, capture_output=True)
        
        print(f"✅ Certificate created: {cert_file}")
    
    # Create SSL context
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(cert_file, key_file)
    
    return context

def main():
    parser = argparse.ArgumentParser(description='Huntmaster Engine Web Server')
    parser.add_argument('--port', type=int, default=8000, help='Server port (default: 8000)')
    parser.add_argument('--host', default='localhost', help='Server host (default: localhost)')
    parser.add_argument('--ssl', action='store_true', help='Enable HTTPS with self-signed certificate')
    parser.add_argument('--cert', help='SSL certificate file')
    parser.add_argument('--key', help='SSL private key file')
    parser.add_argument('--production', action='store_true', help='Production mode (stricter security)')
    
    args = parser.parse_args()
    
    # Check if WASM files exist
    wasm_file = Path('web/dist/huntmaster-engine.wasm')
    if not wasm_file.exists():
        print("⚠️  WASM files not found. Run 'scripts/build_wasm.sh' first.")
        print("   Serving anyway for development...")
    
    # Create server
    with socketserver.TCPServer((args.host, args.port), HuntmasterHandler) as httpd:
        
        if args.ssl:
            # Enable HTTPS
            httpd.socket = create_ssl_context(args.cert, args.key).wrap_socket(
                httpd.socket, server_side=True
            )
            protocol = 'https'
        else:
            protocol = 'http'
        
        print(f"🚀 Huntmaster Engine Server running at {protocol}://{args.host}:{args.port}")
        print(f"   Web Interface: {protocol}://{args.host}:{args.port}/web/")
        print(f"   API Status: {protocol}://{args.host}:{args.port}/api/status")
        print(f"   Master Calls: {protocol}://{args.host}:{args.port}/api/master-calls")
        
        if args.ssl:
            print("🔒 HTTPS enabled (self-signed certificate)")
            print("   Accept the certificate warning in your browser")
        
        print("\n📋 Available endpoints:")
        print("   /web/              - Main testing interface")
        print("   /web/diagnostic.html - Engine diagnostics")
        print("   /api/status        - Engine status (JSON)")
        print("   /api/master-calls  - Available master calls (JSON)")
        print("   /data/master_calls/ - Master call audio files")
        
        print(f"\n🎯 Press Ctrl+C to stop the server")
        
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n👋 Shutting down server...")

if __name__ == '__main__':
    main()
