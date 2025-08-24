import { render, screen } from '@testing-library/react';
import App from './App';

test('renders oscilloscope interface', () => {
  render(<App />);
  const oscilloscopeTitle = screen.getByText(/Digital Oscilloscope/i);
  expect(oscilloscopeTitle).toBeInTheDocument();
});
