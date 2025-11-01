# [20251030-DOCKER-001] GameCalls Engine Backend Dockerfile with Real C++ Engine
# Multi-stage build: C++ engine compilation + Node.js runtime

# Stage 1: Build C++ UnifiedAudioEngine
FROM ubuntu:22.04 AS cpp-builder

WORKDIR /build

# Install C++ build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libsndfile1-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy C++ engine source
COPY src ./src
COPY include ./include
COPY tests ./tests
COPY CMakeLists.txt ./
COPY cmake ./cmake
COPY scripts ./scripts
COPY libs ./libs

# Build UnifiedAudioEngine (no diagnostics variant for production)
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_DIAGNOSTICS=OFF \
          -DBUILD_TESTING=OFF \
          .. && \
    cmake --build . --target UnifiedAudioEngineNoDiag -j$(nproc)

# Stage 2: Build Node-API bindings
FROM node:22-bullseye AS binding-builder

WORKDIR /build

# Install Node-API build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    python3 \
    libsndfile1-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy C++ engine artifacts and headers from previous stage (matching expected paths)
COPY --from=cpp-builder /build/build ./build
COPY --from=cpp-builder /build/include ./include
COPY --from=cpp-builder /build/src ./src
COPY --from=cpp-builder /build/libs ./libs

# Copy binding source and package files
COPY bindings/node-api ./bindings
COPY backend/package*.json ./

# Install node dependencies and build bindings
RUN npm ci && \
    cd bindings && \
    npm install && \
    npm run build

# Stage 3: Production runtime
FROM node:22-bullseye-slim

WORKDIR /app

# Install runtime dependencies
# [20251029-WAVEFORM-021] Added ffmpeg for waveform generation
# [20251030-DOCKER-002] Added libsndfile for C++ engine audio loading
RUN apt-get update && apt-get install -y \
    wget \
    ffmpeg \
    libsndfile1 \
    && rm -rf /var/lib/apt/lists/*

# Copy package files
COPY backend/package*.json ./

# Install Node.js dependencies (production only)
RUN npm ci --only=production

# Copy compiled C++ engine bindings from builder stage
COPY --from=binding-builder /build/bindings/build/Release/gamecalls_engine.node ./bindings/node-api/lib/
COPY --from=cpp-builder /build/build/_deps/kissfft-build/libkissfft-float.so* /usr/local/lib/

# Update library cache for kissfft
RUN ldconfig

# Copy backend source code
COPY backend/src ./src
COPY backend/migrations ./migrations

# Create temp directory for audio processing
# [20251029-WAVEFORM-022] Temp dir for waveform generation
RUN mkdir -p /tmp/gamecalls-waveforms && chmod 1777 /tmp/gamecalls-waveforms

# Set environment
ENV NODE_ENV=production
ENV PORT=5005
ENV USE_REAL_ENGINE=true

# Expose port
EXPOSE 5005

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=45s --retries=3 \
    CMD wget --spider -q http://localhost:5005/health || exit 1

# Start server
CMD ["node", "src/index.js"]

