FROM gcc:latest AS build
RUN apt-get update && apt-get install -y libsqlite3-dev libcrypt-dev
WORKDIR /src
COPY . .
RUN make clean && make

FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends \
    libsqlite3-0 libcrypt1 && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=build /src/teb2 .
COPY --from=build /src/db/schema.sql db/
COPY .env.example .env
EXPOSE 8080
CMD ["./teb2"]
