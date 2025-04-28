import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

# Create output directory if it doesn't exist
output_dir = "plots"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

# Read the solution data
df = pd.read_csv("transport_solution_multiple_mpi.csv")

# Plot the solution at different time steps
plt.figure(figsize=(12, 8))

# Get all column names except 'x'
time_columns = [col for col in df.columns if col != 'x']

# Create a colormap for time evolution
colors = plt.cm.viridis(np.linspace(0, 1, len(time_columns)))

# Plot each time snapshot
for i, col in enumerate(time_columns):
    # Extract time step number from column name
    time_step = int(col.split('_')[1])
    time_value = time_step / len(time_columns) * 1.0  # T=1.0
    
    plt.plot(df['x'], df[col], color=colors[i], 
             label=f't = {time_value:.2f}', linewidth=2)

plt.title('Transport Equation Solution at Different Time Steps (MPI Parallel Version)')
plt.xlabel('x')
plt.ylabel('u(t,x)')
plt.grid(True)
plt.legend(loc='best')

# Save the plot
plot_path = os.path.join(output_dir, "transport_solution_mpi.png")
plt.savefig(plot_path, dpi=300)
plt.close()

print(f"Plot saved to {plot_path}")

# Create an alternative plot with a 3D waterfall view
fig = plt.figure(figsize=(14, 10))
ax = fig.add_subplot(111, projection='3d')

# Extract x and time values
x_values = df['x'].values
time_values = np.linspace(0, 1.0, len(time_columns))

# Create meshgrid for 3D surface
X, T = np.meshgrid(x_values, time_values)
Z = np.zeros_like(X)

# Fill Z values
for i, col in enumerate(time_columns):
    Z[i, :] = df[col].values

# Plot the surface
surf = ax.plot_surface(X, T, Z, cmap='viridis', edgecolor='none', alpha=0.8)

ax.set_xlabel('x')
ax.set_ylabel('t')
ax.set_zlabel('u(t,x)')
ax.set_title('Transport Equation Solution (MPI Parallel Version, 3D View)')

# Add a color bar
fig.colorbar(surf, ax=ax, shrink=0.5, aspect=5)

# Save the 3D plot
plot_path_3d = os.path.join(output_dir, "transport_solution_mpi_3d.png")
plt.savefig(plot_path_3d, dpi=300)
plt.close()

print(f"3D plot saved to {plot_path_3d}")