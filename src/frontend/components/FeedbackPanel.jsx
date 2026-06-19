export default function FeedbackPanel({ feedback }) {
  if (!feedback) {
    return <div className="p-3 bg-gray-50 text-center">No feedback available.</div>;
  }

  // Split the feedback into lines (assuming it's formatted with newlines)
  const lines = feedback.split('\n').filter(line => line.trim() !== '');

  return (
    <div className="p-4 bg-blue-50 border-l-4 border-blue-500">
      {lines.map((line, index) => (
        <div key={index} className="mb-2">
          {/* We can add icons based on the line content, but for simplicity, we'll just show the text */}
          <span className="font-medium">{line}</span>
        </div>
      ))}
    </div>
  );
}
