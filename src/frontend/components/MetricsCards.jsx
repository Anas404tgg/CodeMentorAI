export default function MetricsCards({ title, value }) {
  return (
    <div className="card p-4 text-center">
      <h3 className="text-lg font-semibold mb-2">{title}</h3>
      <p className="text-2xl font-bold">{value}</p>
    </div>
  );
}