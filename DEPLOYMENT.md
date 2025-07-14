# Huntmaster Engine Web Deployment Guide

This guide covers deploying the Huntmaster Engine as a web application for user testing using WebAssembly (WASM).

## Quick Start

### Prerequisites

- Node.js (optional, for advanced server features)
- Python 3.7+ (for development server)
- Git (for submodule management)
- Modern web browser with WASM support

### 1. Build WASM Module

```bash
# Make build script executable
chmod +x scripts/build_wasm.sh

# Build WASM module
./scripts/build_wasm.sh
```

If the build fails due to Emscripten issues, you can use Docker:

```bash
# Build using Docker (fallback)
docker build -f Dockerfile.wasm -t huntmaster-wasm .
docker run -v $(pwd)/web/dist:/output huntmaster-wasm
```

### 2. Start Development Server

```bash
# Start with enhanced CORS support
python serve_production.py

# Or start basic development server
python serve_dev.py
```

### 3. Open Web Interface

Navigate to: `http://localhost:8000/web/user-test.html`

## Project Structure

```
huntmaster-engine/
├── web/                          # Web application
│   ├── user-test.html           # Main testing interface
│   ├── diagnostic.html          # Engine diagnostics
│   ├── index.html              # Original demo
│   ├── styles/                 # CSS stylesheets
│   │   └── main.css           # Main application styles
│   ├── js/                    # JavaScript modules
│   │   ├── main.js           # Main application controller
│   │   ├── audio-utils.js    # Audio processing utilities
│   │   ├── visualization.js  # Real-time visualizations
│   │   ├── performance-monitor.js # Performance tracking
│   │   └── test-manager.js   # Test session management
│   └── dist/                 # Built WASM files (generated)
│       ├── huntmaster-engine.wasm
│       ├── huntmaster-engine.js
│       └── huntmaster-engine.d.ts
├── scripts/
│   └── build_wasm.sh          # WASM build script
├── serve_production.py        # Production web server
├── serve_dev.py              # Development server
├── data/
│   ├── master_calls/         # Master call audio files
│   └── test_audio/          # Test audio files
└── tools/emsdk/             # Emscripten SDK (submodule)
```

## Deployment Options

### Option 1: Local Development Server (Recommended for testing)

```bash
# Enhanced server with API endpoints
python serve_production.py --port 8000

# With HTTPS (generates self-signed certificate)
python serve_production.py --ssl --port 8443

# Production mode (stricter security)
python serve_production.py --production --ssl
```

**Features:**

- ✅ Proper CORS headers for WASM
- ✅ API endpoints for master calls and status
- ✅ HTTPS support with self-signed certificates
- ✅ Performance monitoring
- ✅ File upload handling

### Option 2: Static File Hosting

For simple deployment, you can serve the `web/` directory with any static file server:

```bash
# Using Node.js serve
npx serve web

# Using Python simple server
cd web && python -m http.server 8000

# Using nginx (example configuration below)
```

**Note:** Static hosting requires manual CORS configuration and may not support all features.

### Option 3: Cloud Deployment

#### Netlify/Vercel

1. Push to GitHub repository
2. Connect to Netlify/Vercel
3. Set build command: `./scripts/build_wasm.sh`
4. Set publish directory: `web/`
5. Add custom headers for WASM support

#### AWS S3 + CloudFront

1. Build WASM files locally
2. Upload `web/` directory to S3
3. Configure CloudFront with proper MIME types
4. Set CORS headers for WASM files

#### Docker Deployment

```bash
# Build production Docker image
docker build -t huntmaster-web .

# Run with port mapping
docker run -p 8000:8000 huntmaster-web

# Run with volume for data persistence
docker run -p 8000:8000 -v $(pwd)/data:/app/data huntmaster-web
```

## Configuration

### Environment Variables

```bash
# Server configuration
export HUNTMASTER_PORT=8000
export HUNTMASTER_HOST=0.0.0.0
export HUNTMASTER_SSL=true

# Engine configuration
export HUNTMASTER_SAMPLE_RATE=44100
export HUNTMASTER_FRAME_SIZE=1024
export HUNTMASTER_MFCC_COEFFS=13

# Paths
export HUNTMASTER_DATA_PATH=./data
export HUNTMASTER_WASM_PATH=./web/dist
```

### Server Headers

For WASM to work properly, ensure these headers are set:

```
Cross-Origin-Embedder-Policy: require-corp
Cross-Origin-Opener-Policy: same-origin
Content-Type: application/wasm (for .wasm files)
```

### Nginx Configuration Example

```nginx
server {
    listen 80;
    server_name your-domain.com;
    root /path/to/huntmaster-engine/web;

    # WASM support
    location ~* \.wasm$ {
        add_header Cross-Origin-Embedder-Policy require-corp;
        add_header Cross-Origin-Opener-Policy same-origin;
        add_header Content-Type application/wasm;
    }

    # JavaScript modules
    location ~* \.js$ {
        add_header Content-Type application/javascript;
    }

    # Audio files
    location ~* \.(wav|mp3|ogg)$ {
        add_header Access-Control-Allow-Origin *;
        add_header Cache-Control "public, max-age=3600";
    }

    # Default location
    location / {
        try_files $uri $uri/ /index.html;
        add_header Cross-Origin-Embedder-Policy require-corp;
        add_header Cross-Origin-Opener-Policy same-origin;
    }
}
```

## Testing the Deployment

### 1. Engine Initialization Test

1. Open web interface
2. Click "Initialize Engine"
3. Verify status shows "Engine initialized successfully"

### 2. Master Call Loading Test

1. Select a master call from dropdown
2. Click "Load Call"
3. Verify success message appears

### 3. Live Audio Test

1. Click "Live Microphone"
2. Click "Start Recording"
3. Allow microphone access
4. Verify waveform visualization appears
5. Make sound and verify similarity scores update

### 4. File Upload Test

1. Click "Test Audio Files"
2. Upload a WAV file or select preset
3. Verify processing results appear

### 5. Performance Test

1. Monitor performance metrics during testing
2. Verify memory usage stays reasonable
3. Check for any console errors

## API Endpoints

The production server provides these API endpoints:

### GET /api/status

Returns engine and server status

```json
{
  "status": "ready",
  "version": "1.0.0",
  "wasmSupported": true,
  "buildInfo": { ... }
}
```

### GET /api/master-calls

Returns available master call files

```json
{
  "calls": [
    {
      "name": "buck_grunt",
      "filename": "buck_grunt.wav",
      "path": "/data/master_calls/buck_grunt.wav",
      "size": 1048576
    }
  ]
}
```

### GET /api/test-audio

Returns available test audio files

```json
{
  "testFiles": [
    {
      "name": "test_sample",
      "filename": "test_sample.wav",
      "path": "/data/test_audio/test_sample.wav",
      "size": 524288
    }
  ]
}
```

## Troubleshooting

### WASM Loading Issues

- **Error**: "WASM files not found"
  - **Solution**: Run `./scripts/build_wasm.sh` to build WASM module
  - **Alternative**: Check that `web/dist/huntmaster-engine.wasm` exists

### CORS Errors

- **Error**: "Cross-Origin Request Blocked"
  - **Solution**: Use `serve_production.py` instead of basic static server
  - **Alternative**: Configure your server with proper CORS headers

### Audio Permission Issues

- **Error**: "NotAllowedError: Permission denied"
  - **Solution**: Ensure HTTPS is enabled for microphone access
  - **Alternative**: Use `--ssl` flag with development server

### Performance Issues

- **Issue**: High memory usage
  - **Solution**: Check browser console for memory leaks
  - **Monitor**: Use performance monitor in web interface

### Master Call Loading Issues

- **Error**: "Failed to load master call"
  - **Solution**: Ensure audio files are in `data/master_calls/`
  - **Check**: File format should be WAV, MP3, or OGG

## Browser Compatibility

### Minimum Requirements

- Chrome 69+ (recommended)
- Firefox 78+
- Safari 14+
- Edge 79+

### Required Features

- WebAssembly support
- Web Audio API
- MediaDevices API (for microphone)
- Fetch API
- ES6 modules

### Recommended Features

- SharedArrayBuffer (for best performance)
- AudioWorklet (for low-latency audio)
- Performance Observer API

## Security Considerations

### Development vs Production

- Development: Relaxed CORS, self-signed certificates
- Production: Proper HTTPS, restricted origins

### Data Privacy

- Audio data processed locally in browser
- No audio data transmitted to server
- Test results stored locally only

### Network Security

- Use HTTPS in production
- Validate file uploads
- Sanitize user inputs

## Performance Optimization

### WASM Optimization

- Built with `-O3` optimization
- Uses `--closure 1` for code minification
- Excludes debug symbols in production

### Loading Optimization

- WASM files served with proper caching headers
- JavaScript modules loaded asynchronously
- Audio files cached for repeated testing

### Memory Optimization

- Automatic garbage collection
- Buffer reuse where possible
- Memory pressure monitoring

## Support and Debugging

### Debug Mode

Add `?debug=1` to URL for verbose logging:

```
http://localhost:8000/web/user-test.html?debug=1
```

### Performance Profiling

Use browser dev tools to profile:

1. Open Chrome DevTools
2. Go to Performance tab
3. Record during audio processing
4. Analyze WASM performance

### Log Collection

Export performance metrics and test results:

1. Use "Export Results" button in web interface
2. Check browser console for errors
3. Use performance monitor data

### Getting Help

- Check browser console for errors
- Review this deployment guide
- Test with different browsers
- Verify WASM file integrity
