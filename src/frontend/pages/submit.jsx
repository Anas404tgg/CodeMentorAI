import { useState } from 'react';
import { useRouter } from 'next/router';
import Head from 'next/head';
import MonacoEditor from '@monaco-editor/react';
import FeedbackPanel from '../components/FeedbackPanel';

export default function SubmitCode() {
  const [code, setCode] = useState('#include <stdio.h>\n\nint main() {\n    printf("Hello, World!\n");\n    return 0;\n}');
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
    <div className="container mx-auto px-4 py-8">
      <Head>
        <title>Submit Code - CodeMentor AI</title>
      </Head>
      <main>
        <h1 className="mb-6 text-3xl font-bold text-gray-800">Submit Your C Code</h1>
        <form onSubmit={handleSubmit} className="mb-8 space-y-6">
          <div className="space-y-2">
            <label htmlFor="codeEditor" className="block text-gray-700 font-medium mb-2">
              C Code
            </label>
            <div className="relative">
              <div className="absolute inset-0 pointer-events-none">
                <div className="pointer-events-none absolute inset-0">
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
              </div>
            </div>
          </div>
          <button
            type="submit"
            disabled={loading}
            className={`w-full bg-gradient-to-r from-blue-500 to-blue-600 hover:from-blue-600 hover:to-blue-700 text-white font-semibold py-3 px-6 rounded-lg shadow-lg transition-all duration-200 transform hover:-translate-y-1 hover:scale-105 ${loading ? 'opacity-50' : ''}`}
          >
            {loading ? 'Analyzing...' : 'Analyze Code'}
          </button>
        </form>

        {error && (
          <div className="mt-4 p-4 bg-red-50 border-l-4 border-red-500 text-red-600 rounded-lg">
            {error}
          </div>
        )}

        {result && (
          <div className="bg-white rounded-xl shadow-md overflow-hidden">
            <div className="px-6 py-4 border-b border-gray-200">
              <h2 className="text-xl font-semibold text-gray-800 mb-4">Analysis Results</h2>
            </div>
            <div className="grid grid-cols-1 md:grid-cols-2 gap-6 p-6">
              <div className="space-y-4">
                <h3 className="font-semibold mb-2 text-gray-800">Metrics</h3>
                <div className="space-y-2">
                  <p className="flex justify-between">
                    <span className="text-gray-600">Lines:</span>
                    <span className="font-medium">{result.metrics.lines}</span>
                  </p>
                  <p className="flex justify-between">
                    <span className="text-gray-600">Functions:</span>
                    <span className="font-medium">{result.metrics.functions}</span>
                  </p>
                  <p className="flex justify-between">
                    <span className="text-gray-600">Max Nesting Depth:</span>
                    <span className="font-medium">{result.metrics.max_nesting_depth}</span>
                  </p>
                  <p className="flex justify-between">
                    <span className="text-gray-600">Dangerous Patterns:</span>
                    <span className="font-medium">{result.metrics.dangerous_patterns}</span>
                  </p>
                </div>
              </div>
              <div className="space-y-4">
                <h3 className="font-semibold mb-2 text-gray-800">Sandbox Result</h3>
                <div className="space-y-2">
                  <p className="flex justify-between">
                    <span className="text-gray-600">Exit Code:</span>
                    <span className="font-medium">{result.sandbox_result.exit_code}</span>
                  </p>
                  <p className="flex justify-between">
                    <span className="text-gray-600">Timed Out:</span>
                    <span className="font-medium">{result.sandbox_result.timed_out ? 'Yes' : 'No'}</span>
                  </p>
                  <p className="flex justify-between">
                    <span className="text-gray-600">Stdout:</span>
                    <span className="font-medium ml-2 max-w-xs break-all">{result.sandbox_result.stdout}</span>
                  </p>
                  <p className="flex justify-between">
                    <span className="text-gray-600">Stderr:</span>
                    <span className="font-medium ml-2 max-w-xs break-all">{result.sandbox_result.stderr}</span>
                  </p>
                </div>
              </div>
            </div>
            <div className="px-6 pt-4 pb-6">
              <h3 className="font-semibold mb-2 text-gray-800">AI Feedback</h3>
              <FeedbackPanel feedback={result.ai_feedback} />
            </div>
            <div className="px-6 py-4">
              <button
                onClick={() => router.push('/dashboard')}
                className="w-full bg-gradient-to-r from-green-500 to-green-600 hover:from-green-600 hover:to-green-700 text-white font-semibold py-3 px-6 rounded-lg shadow-lg transition-all duration-200 transform hover:-translate-y-1 hover:scale-105"
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
