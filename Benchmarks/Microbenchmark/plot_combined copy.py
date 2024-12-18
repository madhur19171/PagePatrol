import pandas as pd
import matplotlib.pyplot as plt

# Load the log files into DataFrames
file1 = "page_faults.log"
file2 = "page_faults_pp.log"
data1 = pd.read_csv(file1)
data2 = pd.read_csv(file2)

# Create a dual-axis plot
fig, ax1 = plt.subplots(figsize=(10, 6), dpi=300)  # Higher DPI for better resolution

# Plot Minor Page Faults from page_faults.log on the left y-axis
color1 = 'tab:blue'
ax1.set_xlabel("Log Index")
ax1.set_ylabel("Minor Page Faults", color=color1)
minor1, = ax1.plot(
    data1["Index"],
    data1["Minor Page Faults"],
    label="Minor Page Faults (page_faults)",
    linestyle="--",
    marker="o",
    color=color1,
)
minor2, = ax1.plot(
    data2["Index"],
    data2["Minor Page Faults"],
    label="Minor Page Faults (page_faults_pp)",
    linestyle="-",
    marker="o",
    color=color1,
)
ax1.tick_params(axis='y', labelcolor=color1)

# Create the second y-axis for Major Page Faults
ax2 = ax1.twinx()  # instantiate a second y-axis that shares the same x-axis
color2 = 'tab:red'
ax2.set_ylabel("Major Page Faults", color=color2)
major1, = ax2.plot(
    data1["Index"],
    data1["Major Page Faults"],
    label="Major Page Faults (page_faults)",
    linestyle="--",
    marker="o",
    color=color2,
)
major2, = ax2.plot(
    data2["Index"],
    data2["Major Page Faults"],
    label="Major Page Faults (page_faults_pp)",
    linestyle="-",
    marker="o",
    color=color2,
)
ax2.tick_params(axis='y', labelcolor=color2)

# Combine legends with proper order (blue above red)
ax1.legend([
    minor1, minor2, major1, major2
], [
    "Minor Page Faults (Baseline)",
    "Minor Page Faults (PagePatrol)",
    "Major Page Faults (Baseline)",
    "Major Page Faults (PagePatrol)"
], loc="upper left")

# Add a title and grid
plt.title("Comparison of Page Faults (Minor and Major) Across Logs")
ax1.grid()

# Save the plot to a file
plt.savefig("comparison_page_faults_dual_axis_graph.png", dpi=300)  # Save with high DPI
