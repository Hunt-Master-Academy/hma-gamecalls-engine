# Local Connections Guide (Infra → Engines)

Purpose
- Provide exact connection details for local testing of Hunt-Strategy-Engine and Gamecalls-Engine against the infra stack.
- Document required .env keys, ready-to-copy connection strings, and verification commands.

Prerequisites
- Docker and Docker Compose
- hma-infra/.env configured
- Stack running: docker compose -f docker/docker-compose.yml up -d

Compose file path
- hma-infra/docker/docker-compose.yml

Required .env keys (hma-infra/.env)
- PostgreSQL
  - POSTGRES_DB
  - POSTGRES_USER
  - POSTGRES_PASSWORD
- Redis
  - REDIS_PASSWORD (optional, if configured)
- MinIO (S3)
  - MINIO_ROOT_USER
  - MINIO_ROOT_PASSWORD
- Content Bridge (optional)
  - CONTENT_MODE (local|hybrid|s3)
  - CONTENT_BUCKET (for S3/hybrid)
  - CDN_URL
  - HMA_CONTENT_PATH (host path to hma-content)

Services (Docker network names → host ports)
- Postgres: service=postgres, internal=5432, host-port=discover via command below
- Redis: service=redis, internal=6379, host-port=6379 (default; confirm)
- MinIO (S3): service=minio, S3=9000, Console=9001
- Content Bridge: service=content-bridge, HTTP=8090 (if enabled)

Discover host-mapped ports
- PostgreSQL (host port)
  docker compose -f docker/docker-compose.yml port postgres 5432
- Redis (host port)
  docker compose -f docker/docker-compose.yml port redis 6379

PostgreSQL connection

From host (psql/engines on host):
- Connection string (Gamecalls schema)
  postgresql://POSTGRES_USER:POSTGRES_PASSWORD@localhost:HOST_PORT/POSTGRES_DB?options=-c%20search_path=game_calls
- Connection string (Hunt-Strategy schema)
  postgresql://POSTGRES_USER:POSTGRES_PASSWORD@localhost:HOST_PORT/POSTGRES_DB?options=-c%20search_path=hunt_strategy

From another container on the same Docker network:
- Connection string (set search_path at connect time)
  postgresql://POSTGRES_USER:POSTGRES_PASSWORD@postgres:5432/POSTGRES_DB?options=-c%20search_path=game_calls

Set search_path in session (psql):
- docker compose -f docker/docker-compose.yml exec postgres psql -U "$POSTGRES_USER" -d "$POSTGRES_DB" -c "SET search_path TO game_calls; \\dt"
- Repeat with hunt_strategy

Recommended least-privilege roles (optional)
- SQL (run once)
  CREATE ROLE gamecalls_rw LOGIN PASSWORD 'choose-strong-pass';
  GRANT USAGE ON SCHEMA game_calls TO gamecalls_rw;
  GRANT SELECT,INSERT,UPDATE,DELETE ON ALL TABLES IN SCHEMA game_calls TO gamecalls_rw;
  ALTER DEFAULT PRIVILEGES IN SCHEMA game_calls GRANT SELECT,INSERT,UPDATE,DELETE ON TABLES TO gamecalls_rw;

  CREATE ROLE huntstrategy_rw LOGIN PASSWORD 'choose-strong-pass';
  GRANT USAGE ON SCHEMA hunt_strategy TO huntstrategy_rw;
  GRANT SELECT,INSERT,UPDATE,DELETE ON ALL TABLES IN SCHEMA hunt_strategy TO huntstrategy_rw;
  ALTER DEFAULT PRIVILEGES IN SCHEMA hunt_strategy GRANT SELECT,INSERT,UPDATE,DELETE ON TABLES TO huntstrategy_rw;

Redis connection

From host:
- redis://localhost:6379
- With password (if set): redis://:REDIS_PASSWORD@localhost:6379

From container:
- redis://redis:6379

MinIO (S3) for content/artifacts

Endpoints:
- API: http://localhost:9000
- Console: http://localhost:9001

Credentials (from .env):
- MINIO_ROOT_USER
- MINIO_ROOT_PASSWORD

Example mc setup in container:
- docker compose -f docker/docker-compose.yml exec minio mc alias set local http://localhost:9000 "$MINIO_ROOT_USER" "$MINIO_ROOT_PASSWORD"
- docker compose -f docker/docker-compose.yml exec minio mc ls local

Content Bridge (optional but recommended for engines using content)

Endpoint:
- Health: http://localhost:8090/health
- Audio: http://localhost:8090/api/audio
- Icons: http://localhost:8090/api/icons
- Research: http://localhost:8090/api/research
- Manifest: http://localhost:8090/api/manifest

Backed by:
- Local content path (host): ${HMA_CONTENT_PATH:-../hma-content}
- Mounted read-only inside container at /content

Engine-side environment variables

Common:
- DATABASE_URL=postgresql://USER:PASS@HOST:PORT/DB
- GAMECALLS_DB_SCHEMA=game_calls
- HUNTSTRATEGY_DB_SCHEMA=hunt_strategy
- REDIS_URL=redis://HOST:PORT
- Optional S3:
  - S3_ENDPOINT=http://localhost:9000
  - S3_REGION=us-east-1
  - S3_ACCESS_KEY_ID=$MINIO_ROOT_USER
  - S3_SECRET_ACCESS_KEY=$MINIO_ROOT_PASSWORD
  - S3_BUCKET=hma-content-alpha

Ready-to-copy helpers (Linux)

Export .env and compute host port:
- set -a; source .env; set +a
- HOST_PG_PORT=$(docker compose -f docker/docker-compose.yml port postgres 5432 | awk -F: '{print $2}')

Construct connection strings:
- echo "Gamecalls:  postgresql://$POSTGRES_USER:$POSTGRES_PASSWORD@localhost:$HOST_PG_PORT/$POSTGRES_DB?options=-c%20search_path=game_calls"
- echo "HuntStrategy: postgresql://$POSTGRES_USER:$POSTGRES_PASSWORD@localhost:$HOST_PG_PORT/$POSTGRES_DB?options=-c%20search_path=hunt_strategy"

Verification

Schemas present:
- docker compose -f docker/docker-compose.yml exec postgres psql -U "$POSTGRES_USER" -d "$POSTGRES_DB" -c "SELECT schema_name FROM information_schema.schemata WHERE schema_name IN ('game_calls','hunt_strategy');"

Tables list:
- docker compose -f docker/docker-compose.yml exec postgres psql -U "$POSTGRES_USER" -d "$POSTGRES_DB" -c "SELECT table_schema,table_name FROM information_schema.tables WHERE table_schema IN ('game_calls','hunt_strategy') ORDER BY 1,2;"

Basic query:
- docker compose -f docker/docker-compose.yml exec postgres psql -U "$POSTGRES_USER" -d "$POSTGRES_DB" -c "SET search_path TO game_calls; \\dt"

Troubleshooting

- Check container health:
  docker compose -f docker/docker-compose.yml ps

- Logs:
  docker compose -f docker/docker-compose.yml logs -f postgres

- Network readiness from a container:
  docker compose -f docker/docker-compose.yml exec postgres pg_isready -h postgres -p 5432 -U "$POSTGRES_USER"

Notes
- This guide targets local development. Disable SSL locally (sslmode=disable).
- Replace placeholders with actual values or use the generator script below.

Generator (optional)
- Use scripts/generate-local-connections.sh to render a filled copy with current .env values and host ports into docs/local-connections.filled.md.
```
```bash
// filepath: /home/xbyooki/projects/hma-infra/scripts/generate-local-connections.sh
#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

if [[ ! -f .env ]]; then
  echo "Missing .env in hma-infra. Aborting." >&2
  exit 1
fi

set -a
# shellcheck disable=SC1091
source .env
set +a

COMPOSE_FILE="docker/docker-compose.yml"

# Discover host ports
HOST_PG_PORT="$(docker compose -f "$COMPOSE_FILE" port postgres 5432 2>/dev/null | awk -F: '{print $2}')"
HOST_REDIS_PORT="$(docker compose -f "$COMPOSE_FILE" port redis 6379 2>/dev/null | awk -F: '{print $2}')"
HOST_MINIO_API_PORT="$(docker compose -f "$COMPOSE_FILE" port minio 9000 2>/dev/null | awk -F: '{print $2}')"
HOST_MINIO_CONSOLE_PORT="$(docker compose -f "$COMPOSE_FILE" port minio 9001 2>/dev/null | awk -F: '{print $2}')"
HOST_CONTENT_BRIDGE_PORT="$(docker compose -f "$COMPOSE_FILE" port content-bridge 8090 2>/dev/null | awk -F: '{print $2}')"

OUT="docs/local-connections.filled.md"
mkdir -p docs

cat > "$OUT" <<EOF
# Local Connections (Filled)

Generated: $(date)

## PostgreSQL
- Host: localhost
- Port: ${HOST_PG_PORT:-5432}
- Database: ${POSTGRES_DB:-}
- User: ${POSTGRES_USER:-}
- Password: ${POSTGRES_PASSWORD:-}
- From containers: host=postgres port=5432
- Gamecalls URL:
  postgresql://${POSTGRES_USER}:${POSTGRES_PASSWORD}@localhost:${HOST_PG_PORT:-5432}/${POSTGRES_DB}?options=-c%20search_path=game_calls
- Hunt-Strategy URL:
  postgresql://${POSTGRES_USER}:${POSTGRES_PASSWORD}@localhost:${HOST_PG_PORT:-5432}/${POSTGRES_DB}?options=-c%20search_path=hunt_strategy

## Redis
- Host: localhost
- Port: ${HOST_REDIS_PORT:-6379}
- URL: redis://localhost:${HOST_REDIS_PORT:-6379}
- Password: ${REDIS_PASSWORD:-<none>}

## MinIO (S3)
- API: http://localhost:${HOST_MINIO_API_PORT:-9000}
- Console: http://localhost:${HOST_MINIO_CONSOLE_PORT:-9001}
- Access Key: ${MINIO_ROOT_USER:-}
- Secret Key: ${MINIO_ROOT_PASSWORD:-}
- Suggested bucket: hma-content-alpha

## Content Bridge
- Health: http://localhost:${HOST_CONTENT_BRIDGE_PORT:-8090}/health
- Audio:  http://localhost:${HOST_CONTENT_BRIDGE_PORT:-8090}/api/audio
- Icons:  http://localhost:${HOST_CONTENT_BRIDGE_PORT:-8090}/api/icons
- Research: http://localhost:${HOST_CONTENT_BRIDGE_PORT:-8090}/api/research
- Manifest: http://localhost:${HOST_CONTENT_BRIDGE_PORT:-8090}/api/manifest
- Mode: ${CONTENT_MODE:-local}
- CDN_URL: ${CDN_URL:-http://localhost:8090}
- Local content path (host): ${HMA_CONTENT_PATH:-../hma-content}

## Engine-side env examples
DATABASE_URL=postgresql://${POSTGRES_USER}:${POSTGRES_PASSWORD}@localhost:${HOST_PG_PORT:-5432}/${POSTGRES_DB}
GAMECALLS_DB_SCHEMA=game_calls
HUNTSTRATEGY_DB_SCHEMA=hunt_strategy
REDIS_URL=redis://localhost:${HOST_REDIS_PORT:-6379}
S3_ENDPOINT=http://localhost:${HOST_MINIO_API_PORT:-9000}
S3_REGION=us-east-1
S3_ACCESS_KEY_ID=${MINIO_ROOT_USER:-}
S3_SECRET_ACCESS_KEY=${MINIO_ROOT_PASSWORD:-}
S3_BUCKET=hma-content-alpha

## Verification
docker compose -f docker/docker-compose.yml exec postgres psql -U "${POSTGRES_USER}" -d "${POSTGRES_DB}" -c "SELECT schema_name FROM information_schema.schemata WHERE schema_name IN ('game_calls','hunt_strategy');"
docker compose -f docker/docker-compose.yml exec postgres psql -U "${POSTGRES_USER}" -d "${POSTGRES_DB}" -c "SELECT table_schema,table_name FROM information_schema.tables WHERE table_schema IN ('game_calls','hunt_strategy') ORDER BY 1,2;"
EOF

echo "Wrote $OUT"
```