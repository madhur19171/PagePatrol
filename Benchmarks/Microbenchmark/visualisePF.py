# Import necessary libraries
import re
import matplotlib.pyplot as plt

# Define a function to extract page faults
def extract_page_faults(file_path):
    minor_faults = []
    major_faults = []

    # Open and read the file line by line
    with open(file_path, 'r') as file:
        for line in file:
            # Match and extract Minor Page Faults
            minor_match = re.search(r"Minor Page Faults:\s*(\d+)", line)
            if minor_match:
                minor_faults.append(int(minor_match.group(1)))
            
            # Match and extract Major Page Faults
            major_match = re.search(r"Major Page Faults:\s*(\d+)", line)
            if major_match:
                major_faults.append(int(major_match.group(1)))

    return minor_faults, major_faults

# File paths
basic_file_path = "basic.txt"
with_hints_file_path = "hints-Noreuse.txt"

# Extract data from both files
basic_minor_faults, basic_major_faults = extract_page_faults(basic_file_path)
hints_minor_faults, hints_major_faults = extract_page_faults(with_hints_file_path)

# # Plot Minor Page Faults Comparison
# plt.figure(figsize=(10, 5))
# plt.plot(basic_minor_faults, label="Basic - Minor Page Faults", marker='o')
# plt.plot(hints_minor_faults, label="With Hints - Minor Page Faults", marker='o')
# plt.title("Comparison of Minor Page Faults")
# plt.xlabel("Measurement Index")
# plt.ylabel("Number of Minor Page Faults")
# plt.legend()
# plt.grid()
# plt.show()

# Plot Major Page Faults Comparison
plt.figure(figsize=(10, 5))
plt.plot(basic_major_faults, label="Basic - Major Page Faults", marker='o')
plt.plot(hints_major_faults, label="With Hints - Major Page Faults", marker='o')
plt.title("Comparison of Major Page Faults")
plt.xlabel("Measurement Index")
plt.ylabel("Number of Major Page Faults")
plt.legend()
plt.grid()
plt.show()