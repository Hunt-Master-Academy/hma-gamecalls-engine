#!/usr/bin/env python3
# serve_dev.py - Development server with proper CORS headers for WASM

from http.server import HTTPServer, SimpleHTTPRequestHandler
import sys
import os

class CORSRequestHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        # Required for SharedArrayBuffer and WASM
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        
        # General CORS
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', '*')
        
        # WASM MIME type
        self.send_header('Content-Type', 'application/wasm')
        
        super().end_headers()
    
    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()
    
    def guess_type(self, path):
        mimetype = super().guess_type(path)
        if path.endswith('.wasm'):
            return 'application/wasm'
        return mimetype

if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
    
    # Change to web directory
    web_dir = os.path.join(os.path.dirname(__file__), 'web')
    if os.path.exists(web_dir):
        os.chdir(web_dir)
    
    server = HTTPServer(('localhost', port), CORSRequestHandler)
    print(f"🦌 Huntmaster Dev Server running at http://localhost:{port}/")
    print(f"📁 Serving from: {os.getcwd()}")
    print("Press Ctrl+C to stop...")
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n👋 Server stopped.")