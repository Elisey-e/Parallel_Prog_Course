import pandas as pd
import matplotlib.pyplot as plt
import os

# Create output directory if it doesn't exist
output_dir = "plots"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

# Read the solution data
df = pd.read_csv("transport_solution.csv")

# Plot the initial and final states
plt.figure(figsize=(10, 6))
plt.plot(df['x'], df['u_initial'], 'b-', label='t = 0')
plt.plot(df['x'], df['u_final'], 'r-', label=f't = {1.0}')

plt.title('Transport Equation Solution')
plt.xlabel('x')
plt.ylabel('u(t,x)')
plt.grid(True)
plt.legend()

# Save the plot
plot_path = os.path.join(output_dir, "transport_solution_plot.png")
plt.savefig(plot_path)
plt.close()

print(f"Plot saved to {plot_path}")
