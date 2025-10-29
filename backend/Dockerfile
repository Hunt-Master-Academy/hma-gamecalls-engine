# [20251028-DOCKER-001] GameCalls Engine Backend Dockerfile
# Simplified build without C++ bindings (mock mode for initial testing)

FROM node:22-bullseye-slim

WORKDIR /app

# Install runtime dependencies
# [20251029-WAVEFORM-021] Added ffmpeg for waveform generation
RUN apt-get update && apt-get install -y \
    wget \
    ffmpeg \
    && rm -rf /var/lib/apt/lists/*

# Copy package files
COPY package*.json ./

# Install Node.js dependencies
RUN npm ci --only=production

# Copy source code
COPY src ./src
COPY migrations ./migrations

# Create temp directory for audio processing
# [20251029-WAVEFORM-022] Temp dir for waveform generation
RUN mkdir -p /tmp/gamecalls-waveforms && chmod 1777 /tmp/gamecalls-waveforms

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
