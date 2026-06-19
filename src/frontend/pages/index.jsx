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
      <main className="min-h-screen bg-gradient-to-br from-gray-50 to-gray-100">
        <div className="container mx-auto px-4 py-12">
          <div className="max-w-2xl mx-auto bg-white rounded-xl shadow-2xl p-8">
            <div className="text-center mb-8">
              <h1 className="gradient-text text-4xl font-bold mb-4">
                CodeMentor AI
              </h1>
              <p className="text-gray-600 lg:text-base">
                An educational platform that helps you learn C programming by providing
                AI-powered feedback on your code submissions.
              </p>
            </div>
            
            <form onSubmit={handleSubmit} className="space-y-6">
              <div className="space-y-2">
                <label htmlFor="username" className="block text-gray-700 font-medium">
                  Username
                </label>
                <input
                  type="text"
                  id="username"
                  value={username}
                  onChange={(e) => setUsername(e.target.value)}
                  placeholder="Enter your username"
                  className="w-full px-4 py-3 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent outline-none transition-all duration-200 bg-gray-50"
                  autoFocus
                />
                {error && (
                  <p className="text-red-500 text-sm mt-1">
                    {error}
                  </p>
                )}
              </div>
              <button
                type="submit"
                className="w-full bg-gradient-to-r from-blue-500 to-blue-600 hover:from-blue-600 hover:to-blue-700 text-white font-semibold py-3 px-6 rounded-lg shadow-lg transition-all duration-200 transform hover:-translate-y-1 hover:scale-105 disabled:opacity-50 disabled:shadow-none"
              >
                Get Started
              </button>
            </form>
          </div>
        </div>
      </main>
    </>
  );
}
