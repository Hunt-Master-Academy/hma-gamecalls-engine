# [20251028-DOCKER-001] GameCalls Engine Backend Dockerfile
# Multi-stage build for Node.js + C++ native bindings

# Stage 1: Build environment with C++ toolchain
FROM node:22-bullseye AS builder

WORKDIR /app

# Install C++ build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    python3 \
    git \
    && rm -rf /var/lib/apt/lists/*

# Copy package files
COPY package*.json ./

# Install Node.js dependencies (including node-gyp for native addons)
RUN npm ci --production=false

# Copy source code and bindings
COPY . .

# Build C++ native bindings if build scripts exist
RUN if [ -d "bindings/node-api" ]; then \
        cd bindings/node-api && \
        npm install && \
        npm run build; \
    fi

# Stage 2: Production runtime
FROM node:22-bullseye-slim

WORKDIR /app

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    wget \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

# Copy from builder
COPY --from=builder /app/node_modules ./node_modules
COPY --from=builder /app/package*.json ./
COPY --from=builder /app/src ./src
COPY --from=builder /app/bindings ./bindings
COPY --from=builder /app/migrations ./migrations

# Create temp directory for audio processing
RUN mkdir -p /tmp/gamecalls && chmod 1777 /tmp/gamecalls

# Set environment
ENV NODE_ENV=production
ENV PORT=5005

# Expose port
EXPOSE 5005

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=45s --retries=3 \
    CMD wget --spider -q http://localhost:5005/health || exit 1

# Start server
CMD ["node", "src/index.js"]
