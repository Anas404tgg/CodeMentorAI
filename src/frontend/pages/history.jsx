import { useEffect, useState } from 'react';
import { useRouter } from 'next/router';
import Head from 'next/head';

export default function History() {
  const [history, setHistory] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');
  const router = useRouter();

  useEffect(() => {
    // Fetch history from the backend API
    const fetchHistory = async () => {
      try {
        // Get username from localStorage (set in login page)
        const username = localStorage.getItem('username') || 'demo_user';

        // Fetch history from backend
        const response = await fetch('http://localhost:8083/api/history', {
          method: 'GET',
          headers: {
            'X-Username': username,
          },
        });

        if (!response.ok) {
          throw new Error(`Server returned ${response.status}`);
        }

        const historyData = await response.json();

        // The backend returns { history: [...] }
        if (historyData.history) {
          setHistory(historyData.history);
        } else {
          setHistory([]);
        }
        setLoading(false);
      } catch (err) {
        console.error('Error fetching history:', err);
        setError('Failed to load history from backend.');
        setLoading(false);
      }
    };

    fetchHistory();
  }, []);

  if (loading) {
    return <div className="container">Loading...</div>;
  }

  if (error) {
    return <div className="container p-4">{error}</div>;
  }

  return (
    <div className="container mx-auto px-4 py-8">
      <Head>
        <title>History - CodeMentor AI</title>
      </Head>
      <main>
        <h1 className="mb-6 text-3xl font-bold text-gray-800">Submission History</h1>
        {history.length === 0 ? (
          <p className="text-center py-8">No submissions yet.</p>
        ) : (
          <table className="min-w-full bg-white border border-gray-200">
            <thead>
              <tr className="bg-gray-100">
                <th className="px-4 py-2 text-left">#</th>
                <th className="px-4 py-2 text-left">Date</th>
                <th className="px-4 py-2 text-left">Lines</th>
                <th className="px-4 py-2 text-left">Functions</th>
                <th className="px-4 py-2 text-left">Score</th>
                <th className="px-4 py-2 text-left">Actions</th>
              </tr>
            </thead>
            <tbody>
              {history.map((item) => (
                <tr key={item.submission_id} className="border-t">
                  <td className="px-4 py-2">{item.submission_id}</td>
                  <td className="px-4 py-2">{item.submitted_at}</td>
                  <td className="px-4 py-2">{item.lines}</td>
                  <td className="px-4 py-2">{item.functions}</td>
                  <td className="px-4 py-2">
                    {/* Calculate a simple score: 100 - (dangerous_patterns * 10) - (max_nesting_depth * 5) */}
                    {Math.max(0, 100 - item.dangerous_patterns * 10 - item.max_nesting_depth * 5)}
                  </td>
                  <td className="px-4 py-2">
                    <button
                      onClick={() => {
                        // In a real app, we would navigate to a detail view or submit page with the code pre-filled.
                        // For now, we'll just show an alert.
                        alert('View submission details');
                      }}
                      className="btn btn-sm"
                    >
                      View
                    </button>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </main>
    </div>
  );
}
