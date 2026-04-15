FROM gcc:latest AS build
RUN apt-get update && apt-get install -y libsqlite3-dev libcrypt-dev
WORKDIR /src
COPY . .
RUN make clean && make

FROM debian:bookworm-slim
RUN apt-get update && \
    apt-get install -y --no-install-recommends libsqlite3-0 libcrypt1 curl && \
    rm -rf /var/lib/apt/lists/* && \
    groupadd -r teb2 && useradd -r -g teb2 -d /app -s /sbin/nologin teb2
WORKDIR /app
COPY --from=build /src/teb2 .
COPY ui/ ui/
RUN mkdir -p /app/data && chown -R teb2:teb2 /app
USER teb2
ENV DB_PATH=/app/data/teb2.db
ENV PORT=8080
EXPOSE 8080
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -sf http://localhost:8080/healthz || exit 1
CMD ["./teb2"]
