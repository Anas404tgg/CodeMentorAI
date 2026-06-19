export default function MetricsCards({ title, value }) {
  return (
    <div className="bg-white rounded-xl shadow-md overflow-hidden">
      <div className="px-6 py-4 border-b border-gray-200">
        <h3 className="text-lg font-semibold mb-2 text-gray-800">{title}</h3>
      </div>
      <div className="px-6 pt-6 pb-4">
        <p className="text-3xl font-bold text-center">{value}</p>
      </div>
    </div>
  );
}
