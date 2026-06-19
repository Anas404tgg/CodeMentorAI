import { useState } from 'react';
import { useRouter } from 'next/router';
import Head from 'next/head';

export default function Home() {
  const [username, setUsername] = useState('');
  const [error, setError] = useState('');
  const router = useRouter();

  const handleSubmit = (e) => {
    e.preventDefault();
    if (!username.trim()) {
      setError('Please enter a username');
      return;
    }
    // In a real app, we would validate the user with the backend.
    // For now, we'll store the username in localStorage and navigate to dashboard.
    localStorage.setItem('username', username);
    router.push('/dashboard');
  };

  return (
    <>
      <Head>
        <title>CodeMentor AI</title>
        <meta name="description" content="CodeMentor AI - Learn C programming with AI feedback" />
      </Head>
      <main className="container">
        <section className="card">
          <h1>CodeMentor AI</h1>
          <p>
            An educational platform that helps you learn C programming by providing
            AI-powered feedback on your code submissions.
          </p>
          <form onSubmit={handleSubmit} className="mt-4">
            <div className="mb-3">
              <label htmlFor="username" className="mb-1 block">
                Username
              </label>
              <input
                type="text"
                id="username"
                value={username}
                onChange={(e) => setUsername(e.target.value)}
                placeholder="Enter your username"
                className="w-full px-3 py-2 border border-gray-300 rounded"
                autoFocus
              />
              {error && <p className="text-red-500 mt-1">{error}</p>}
            </div>
            <button type="submit" className="btn w-full">
              Get Started
            </button>
          </form>
        </section>
      </main>
    </>
  );
}