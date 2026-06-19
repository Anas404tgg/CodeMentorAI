# Multi-stage Docker build for CodeMentor AI Backend

# Stage 1: Build the backend
FROM gcc:11.2 as builder

# Install dependencies
RUN apt-get update && apt-get install -y \
    libmicrohttpd-dev \
    libcurl4-openssl-dev \
    libsqlite3-dev \
    libcjson-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Build the backend
RUN make clean && make

# Stage 2: Create the runtime image
FROM debian:bullseye-slim

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libmicrohttpd11 \
    libcurl4 \
    libsqlite3-0 \
    libcjson1 \
    && rm -rf /var/lib/apt/lists/*

# Copy the backend executable from the builder
COPY --from=builder /app/codementor /usr/local/bin/codementor

# Expose the port the backend listens on
EXPOSE 8080

# Set environment variables (if any) - the user should set GEMINI_API_KEY at runtime
ENV PORT=8080

# Run the backend
CMD ["codementor"]