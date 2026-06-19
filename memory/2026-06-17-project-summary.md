---
name: project-summary
description: Summary of CodeMentor AI project scope and clarifying questions
metadata:
  type: project
---

# Project Summary
CodeMentor AI is a full-stack educational platform where students submit C code via a React frontend; a C backend (libmicrohttpd) parses the code, runs it in a fork/exec sandbox, and sends code, metrics, and errors to an AI API (Gemini/OpenAI) with a Socratic prompt. The platform stores submissions in SQLite (dev) / Supabase (prod) and displays feedback, metrics, and history on a dashboard. All components follow mandatory constraints: C99 backend, libcurl for HTTP, cJSON for JSON, proper error handling, environment-variable secrets, and GitHub Actions CI/CD pipeline ending with Vercel (frontend) and Docker (backend) deployment.

# Clarifying Questions
1. AI provider: Should we use Gemini Flash (free tier) or OpenAI for AI integration?
2. Sandbox strictness: Besides 5-second SIGALRM timeout, do we want additional limits (ulimit for memory, disabling network, dropping privileges) or is basic fork+execve sufficient?
3. Database tables: Should we include the quizzes table now, or start with users, submissions, analyses and add quizzes later?

# Build Order Confirmation
Estimated line counts per module provided; total C backend ~1050 LOC, frontend React ~560 LOC. Ready to proceed with Phase 1, Module 1 (src/backend/main.c).