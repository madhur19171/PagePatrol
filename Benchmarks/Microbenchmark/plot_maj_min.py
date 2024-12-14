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
ax1.set_xlabel("index")
ax1.set_ylabel("#Major Page Faults")
minor1, = ax1.plot(
    data1["Index"],
    data1["Major Page Faults"],
    linestyle="-",
    label="baseline",
    marker="o",
    color="blue"
)
minor2, = ax1.plot(
    data2["Index"],
    data2["Major Page Faults"],
    label="pagepatrol",
    linestyle="-",
    marker="o",
    color="red"
)

# Combine legends with proper order (blue above red)
ax1.legend(loc="upper left")

# Add a title and grid
plt.title("Comparison of Major Page Faults Across Logs")
ax1.grid()

# Save the plot to a file
plt.savefig("comparison_page_faults_dual_axis_graph.png", dpi=300)  # Save with high DPI
