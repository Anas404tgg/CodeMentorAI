import { useState } from 'react';
import { useRouter } from 'next/router';
import Head from 'next/head';
import MonacoEditor from '@monaco-editor/react';
import FeedbackPanel from '../components/FeedbackPanel';

export default function SubmitCode() {
  const [code, setCode] = useState('#include <stdio.h>\n\nint main() {\n    printf("Hello, World!\\n");\n    return 0;\n}');
  const [language, setLanguage] = useState('c');
  const [result, setResult] = useState(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');
  const router = useRouter();

  const handleSubmit = async (e) => {
    e.preventDefault();
    setLoading(true);
    setError('');
    setResult(null);

    // Get username from localStorage (set in login page)
    const username = localStorage.getItem('username') || 'demo_user';

    try {
      const response = await fetch('http://localhost:8083/api/analyze', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'X-Username': username,
        },
        body: JSON.stringify({
          code: code,
          language: language,
        }),
      });

      if (!response.ok) {
        throw new Error(`Server returned ${response.status}`);
      }

      const result = await response.json();
      setResult(result);
    } catch (err) {
      console.error('Error:', err);
      setError('Failed to analyze code. Please make sure the backend is running.');
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="container">
      <Head>
        <title>Submit Code - CodeMentor AI</title>
      </Head>
      <main>
        <h1 className="mb-4">Submit Your C Code</h1>
        <form onSubmit={handleSubmit} className="mb-6">
          <div className="mb-4">
            <label htmlFor="codeEditor" className="mb-1 block">
              C Code
            </label>
            <MonacoEditor
              height="400px"
              defaultLanguage="c"
              value={code}
              onChange={(newCode) => {
                setCode(newCode);
              }}
              // Additional props to make it fit the container
              options={{
                theme: 'vs-dark',
                automaticLayout: true,
              }}
            />
          </div>
          <button
            type="submit"
            disabled={loading}
            className={`btn w-full ${loading ? 'opacity-50' : ''}`}
          >
            {loading ? 'Analyzing...' : 'Analyze Code'}
          </button>
        </form>

        {error && <div className="mb-4 p-3 bg-red-50 border border-red-200 text-red-600">{error}</div>}

        {result && (
          <div className="card">
            <h2 className="mb-2">Analysis Results</h2>
            <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
              <div>
                <h3 className="font-semibold mb-1">Metrics</h3>
                <p>Lines: {result.metrics.lines}</p>
                <p>Functions: {result.metrics.functions}</p>
                <p>Max Nesting Depth: {result.metrics.max_nesting_depth}</p>
                <p>Dangerous Patterns: {result.metrics.dangerous_patterns}</p>
              </div>
              <div>
                <h3 className="font-semibold mb-1">Sandbox Result</h3>
                <p>Exit Code: {result.sandbox_result.exit_code}</p>
                <p>Timed Out: {result.sandbox_result.timed_out ? 'Yes' : 'No'}</p>
                <p>Stdout: {result.sandbox_result.stdout}</p>
                <p>Stderr: {result.sandbox_result.stderr}</p>
              </div>
            </div>
            <div className="mt-4">
              <h3 className="font-semibold mb-1">AI Feedback</h3>
              <FeedbackPanel feedback={result.ai_feedback} />
            </div>
            <div className="mt-4">
              <button
                onClick={() => router.push('/dashboard')}
                className="btn"
              >
                Go to Dashboard
              </button>
            </div>
          </div>
        )}
      </main>
    </div>
  );
}