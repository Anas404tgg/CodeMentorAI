import { useEffect, useState } from 'react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer } from 'recharts';
import MetricsCards from '../components/MetricsCards';
import FeedbackPanel from '../components/FeedbackPanel';

export default function Dashboard() {
  const [metrics, setMetrics] = useState({
    lines: 0,
    functions: 0,
    maxNestingDepth: 0,
    dangerousPatterns: 0,
    score: 0, // We'll compute a score based on metrics
  });
  const [complexityData, setComplexityData] = useState([]); // Array of { date, value }
  const [feedback, setFeedback] = useState('');
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    // Fetch data from the backend API
    const fetchData = async () => {
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

        const history = await response.json();

        if (history.history && history.history.length > 0) {
          // We have history data
          const latest = history.history[0]; // Most recent submission (assuming sorted by date descending)

          // Set metrics from latest submission
          setMetrics({
            lines: latest.lines || 0,
            functions: latest.functions || 0,
            maxNestingDepth: latest.max_nesting_depth || 0,
            dangerousPatterns: latest.dangerous_patterns || 0,
            score: Math.max(0, 100 - (latest.dangerous_patterns || 0) * 10 - (latest.max_nesting_depth || 0) * 5),
          });

          // Set complexity data: lines of code over time (last 6 submissions)
          const recentSubmissions = history.history.slice(0, 6).reverse(); // Oldest first for chart
          setComplexityData(
            recentSubmissions.map((sub, index) => ({
              date: `Sub${index + 1}`, // Simplified - in real app, use actual date
              value: sub.lines || 0,
            }))
          );

          // Set feedback from latest submission
          setFeedback(latest.ai_feedback || "No feedback available");
        } else {
          // No history yet, show default/empty state
          setMetrics({
            lines: 0,
            functions: 0,
            maxNestingDepth: 0,
            dangerousPatterns: 0,
            score: 0,
          });
          setComplexityData([]);
          setFeedback("Submit some code to see AI feedback here.");
        }
      } catch (err) {
        console.error('Error fetching dashboard data:', err);
        // Fallback to mock data on error for now
        setMetrics({
          lines: 42,
          functions: 5,
          maxNestingDepth: 3,
          dangerousPatterns: 0,
          score: 85,
        });
        setComplexityData([
          { date: 'Jan', value: 60 },
          { date: 'Feb', value: 55 },
          { date: 'Mar', value: 70 },
          { date: 'Apr', value: 65 },
          { date: 'May', value: 80 },
          { date: 'Jun', value: 85 },
        ]);
        setFeedback(
          "🔍 Où est le problème\n" +
          "💡 Le concept à comprendre\n" +
          "🛠 Un indice sous forme de question"
        );
      } finally {
        setLoading(false);
      }
    };

    fetchData();
  }, []);

  if (loading) {
    return <div className="container">Loading...</div>;
  }

  return (
    <div className="container mx-auto px-4 py-8">
      <h1 className="mb-6 text-3xl font-bold text-gray-800">Dashboard</h1>
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-5 mb-6">
        <MetricsCards title="Lines of Code" value={metrics.lines} />
        <MetricsCards title="Functions" value={metrics.functions} />
        <MetricsCards title="Max Nesting Depth" value={metrics.maxNestingDepth} />
        <MetricsCards title="Score" value={metrics.score} />
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        <div className="col-span-1 lg:col-span-2">
          <div className="bg-white rounded-xl shadow-md overflow-hidden">
            <div className="px-6 py-4 border-b border-gray-200">
              <h2 className="text-xl font-semibold text-gray-800">Complexity Over Time</h2>
            </div>
            <div className="p-6">
              <div className="h-64 w-full">
                <ResponsiveContainer width="100%" height="100%">
                  <LineChart data={complexityData}>
                    <CartesianGrid strokeDasharray="3 3" />
                    <XAxis dataKey="date" />
                    <YAxis />
                    <Tooltip />
                    <Legend />
                    <Line type="monotone" dataKey="value" stroke="#8884d8" activeDot={{ r: 8 }} />
                  </LineChart>
                </ResponsiveContainer>
              </div>
            </div>
          </div>
        </div>
        <div className="col-span-1 lg:col-span-1">
          <div className="bg-white rounded-xl shadow-md overflow-hidden">
            <div className="px-6 py-4 border-b border-gray-200">
              <h2 className="text-xl font-semibold text-gray-800">AI Feedback</h2>
            </div>
            <div className="p-6">
              <FeedbackPanel feedback={feedback} />
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
