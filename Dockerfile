FROM gcc:latest AS build
RUN apt-get update && apt-get install -y libsqlite3-dev libcrypt-dev libssl-dev
WORKDIR /src
COPY . .
RUN make clean && make

FROM debian:bookworm-slim
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        libsqlite3-0 libcrypt1 libssl3 curl ca-certificates gnupg && \
    curl -fsSL https://deb.nodesource.com/setup_20.x | bash - && \
    apt-get install -y --no-install-recommends nodejs && \
    rm -rf /var/lib/apt/lists/* && \
    groupadd -r teb2 && useradd -r -g teb2 -d /app -s /sbin/nologin teb2
WORKDIR /app
COPY --from=build /src/teb2 .
COPY exec/browser_worker.js ./browser_worker.js
COPY ui/ ui/
RUN mkdir -p /app/data && chown -R teb2:teb2 /app
# Install Playwright Chromium (optional — browser automation)
RUN npm install playwright 2>/dev/null || true
RUN npx playwright install chromium 2>/dev/null || true
USER teb2
ENV DB_PATH=/app/data/teb2.db
ENV PORT=8080
ENV WORKERS=4
EXPOSE 8080
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -sf http://localhost:8080/healthz || exit 1
CMD ["./teb2"]
