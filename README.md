# CodeMentor AI

[![CI](https://github.com/your-username/project-groupe-codementor-ai/actions/workflows/ci.yml/badge.svg)](https://github.com/your-username/project-groupe-codementor-ai/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Made with C](https://img.shields.io/badge/Made%20with-C-blue)](https://en.wikipedia.org/wiki/C_(programming_language))

CodeMentor AI is an educational web platform that helps students learn C programming by providing AI-powered feedback on their code submissions. The platform features a C-powered backend that parses code, executes it in a sandbox, and integrates with the Gemini AI API to deliver Socratic-style feedback.

## Table of Contents
- [Features](#features)
- [Architecture](#architecture)
- [Technology Stack](#technology-stack)
- [Getting Started](#getting-started)
- [Backend Deployment](#backend-deployment)
- [Frontend Deployment](#frontend-deployment)
- [API Documentation](#api-documentation)
- [Testing](#testing)
- [Team](#team)
- [License](#license)

## Features

- **C Source Code Analysis**: Submit C code via a web interface.
- **Static Code Metrics**: Computes lines, functions, nesting depth, and detects dangerous patterns (e.g., `gets`, `strcpy`).
- **Secure Sandbox Execution**: Compiles and runs code in a lightweight sandbox using `fork/execve` with CPU and memory limits.
- **AI-Powered Feedback**: Sends code, metrics, and execution results to the Gemini API with a strict Socratic prompt to guide student learning.
- **Progress Tracking**: Tracks student performance (bugs solved, quality score, streaks) in a SQLite (dev) / Supabase (prod) database.
- **Real-time Dashboard**: Visualizes metrics over time and displays AI feedback.
- **Submission History**: Review past submissions and feedback.
- **Dockerized Backend**: Easy deployment with Docker Compose.
- **CI/CD Pipeline**: GitHub Actions workflow for building, testing, and linting.

## Architecture

![Architecture Diagram](docs/architecture-diagram.png)

The system follows a strict separation between the backend (C) and frontend (React/Next.js) communicating via REST API/JSON.

### Backend (C)
- **HTTP Server**: `libmicrohttpd` listening on port 8080.
- **Static Parser**: Native C char-by-char parser for code metrics.
- **Sandbox**: `fork/execve` with `SIGALRM` timeout (5 seconds) and `ulimit` for memory.
- **Database**: SQLite3 (dev) / Supabase REST API (prod) via `libcurl`.
- **AI Integration**: `libcurl` to call Gemini Flash (or OpenAI) API with a Socratic prompt.
- **JSON Handling**: `cJSON` for parsing and generating JSON.

### Frontend (React/Next.js)
- **Pages**: Landing/login, dashboard, code submission, history.
- **Components**: Monaco Editor for code input, Recharts for metric visualization, custom feedback panel.
- **State Management**: React hooks (`useState`, `useEffect`).
- **Styling**: CSS modules (or CSS framework of choice).

## Technology Stack

### Backend
- **Language**: C99
- **HTTP Server**: libmicrohttpd
- **HTTP Client**: libcurl
- **JSON**: cJSON
- **Database**: SQLite3 (dev), Supabase (prod)
- **Sandbox**: POSIX `fork/execve`, `setrlimit`, `SIGALRM`
- **Build System**: GNU Make
- **Containerization**: Docker

### Frontend
- **Framework**: Next.js 13+, React 18
- **Code Editor**: @monaco-editor/react
- **Charts**: Recharts
- **Styling**: CSS (customizable)
- **Deployment**: Vercel

## Getting Started

### Prerequisites
- **Backend**: 
  - GCC (with C99 support)
  - libmicrohttpd-dev
  - libcurl4-openssl-dev
  - libsqlite3-dev
  - libcjson-dev
  - GNU Make
- **Frontend**:
  - Node.js (>=16) and npm
  - (Optional) Docker for backend deployment

### Backend Development
1. Clone the repository.
2. Install the dependencies (see above).
3. Build the backend:
   ```bash
   make
   ```
4. Set the Gemini API key (required for AI feedback):
   ```bash
   export GEMINI_API_KEY=your_api_key_here
   ```
5. Run the backend:
   ```bash
   ./codementor
   ```
   The backend will listen on port 8080 (or the value of the `PORT` environment variable).

### Frontend Development
1. Navigate to the `src/frontend` directory.
2. Install dependencies:
   ```bash
   npm install
   ```
3. Start the development server:
   ```bash
   npm run dev
   ```
4. Open [http://localhost:3000](http://localhost:3000) in your browser.

## Backend Deployment

### Using Docker Compose
1. Copy `docker-compose.yml` to a directory with your data.
2. Set the `GEMINI_API_KEY` environment variable (in a `.env` file or export).
3. Start the services:
   ```bash
   docker-compose up -d
   ```
4. The backend will be accessible at [http://localhost:8080](http://localhost:8080).

### Building the Docker Image
```bash
docker build -t codementor-ai-backend .
```

### Running the Docker Container
```bash
docker run -d -p 8080:8080 -e GEMINI_API_KEY=your_api_key_here --name codementor codementor-ai-backend
```

## Frontend Deployment

The frontend is designed to be deployed on [Vercel](https://vercel.com). Push the `src/frontend` directory to a GitHub repository and import it into Vercel.

## API Documentation

### `POST /api/analyze`
Analyzes a C code submission.

**Request Body**
```json
{
  "code": "string"
}
```

**Response**
```json
{
  "metrics": {
    "lines": integer,
    "functions": integer,
    "max_nesting_depth": integer,
    "dangerous_patterns": integer
  },
  "sandbox_result": {
    "exit_code": integer,
    "timed_out": boolean,
    "stdout": string,
    "stderr": string
  },
  "ai_feedback": string
}
```

### `GET /api/history`
Retrieves the history of submissions for the current user (based on session or token; in the mock, we return all).

**Response**
```json
[
  {
    "submission_id": integer,
    "code": string,
    "submitted_at": string (timestamp),
    "lines": integer,
    "functions": integer,
    "max_nesting_depth": integer,
    "dangerous_patterns": integer,
    "exit_code": integer,
    "timed_out": boolean,
    "stdout": string,
    "stderr": string,
    "ai_feedback": string,
    "analyzed_at": string (timestamp)
  }
]
```

### `GET /api/user/:id`
Retrieves a user by ID.

**Response**
```json
{
  "id": integer,
  "username": string,
  "created_at": string (timestamp)
}
```

## Testing

### Unit Tests
The parser unit tests can be run with:
```bash
gcc -std=c99 -Wall -Wextra -I src/backend tests/test_parser.c src/backend/parser.c -o tests/test_parser
./tests/test_parser
```

### Integration Tests
The integration tests require a running backend. Execute:
```bash
chmod +x tests/test_api.sh
./tests/test_api.sh
```
(The script starts the backend, runs the tests, and stops the backend.)

### Full Test Suite (via Makefile)
We don't have a `make test` target yet, but you can run the above commands manually.

## Team

- **Student 1**: [Name] - Backend C developer
- **Student 2**: [Name] - Backend C developer / AI integration
- **Student 3**: [Name] - Frontend React/Next.js developer
- **Student 4**: [Name] - Frontend developer / DevOps

*(Replace with actual names and roles.)*

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Thanks to the professors and directors of ESISA Fès for their guidance.
- Special thanks to Prof. Chafik Boulealam for emphasizing the importance of C systems programming.

---
*Note: This is a university project for ESISA Fès, deadline 30/05/2026.*