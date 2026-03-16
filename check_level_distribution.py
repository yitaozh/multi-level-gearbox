import os
import glob
import sys
import argparse
from collections import defaultdict

def analyze_trace_levels(directory, trace_files=None):
    """
    Analyzes level distribution in trace files within a directory.
    If trace_files list is provided, only analyzes those specific files.
    Otherwise, analyzes all .tr files in the directory.
    """
    print(f"\nAnalyzing directory: {directory}")
    
    if not os.path.exists(directory):
        print(f"Error: Directory not found: {directory}")
        return

    # Determine which files to process
    files_to_process = []
    if trace_files:
        # User specified specific filenames
        for fname in trace_files:
            fpath = os.path.join(directory, fname)
            if os.path.exists(fpath):
                files_to_process.append(fpath)
            else:
                print(f"Warning: Specific file not found: {fpath}")
    else:
        # Process all .tr files in directory
        files_to_process = sorted(glob.glob(os.path.join(directory, "*.tr")))
    
    if not files_to_process:
        print("No trace files found to analyze.")
        return

    for filepath in files_to_process:
        filename = os.path.basename(filepath)
        print(f"\n--- Analysis for {filename} ---")
        
        level_counts = defaultdict(int)
        total_flows = 0
        
        try:
            with open(filepath, 'r') as f:
                for line in f:
                    parts = line.strip().split()
                    if not parts:
                        continue
                    
                    # Format: <flow_id> <src> <dst> <pg> <type> <start_time> <end_time> <level>
                    # Level is the last column (index 7 or -1)
                    try:
                        level = int(parts[-1])
                        level_counts[level] += 1
                        total_flows += 1
                    except ValueError:
                        continue
                            
            print(f"Total Flows: {total_flows}")
            
            if total_flows > 0:
                print("Level Distribution:")
                # Sort by level
                sorted_levels = sorted(level_counts.keys())
                
                for level in sorted_levels:
                    count = level_counts[level]
                    percentage = (count / total_flows) * 100
                    print(f"  Level {level}: {count} flows ({percentage:.2f}%)")
                    
                # Calculate "Downgraded" flows (Level > 0)
                fastest_level_count = level_counts.get(0, 0)
                downgraded_count = total_flows - fastest_level_count
                downgraded_percentage = (downgraded_count / total_flows) * 100
                
                print(f"-> Flows kept in Level 0 (Fastest): {fastest_level_count} ({100-downgraded_percentage:.2f}%)")
                print(f"-> Flows downgraded (Level > 0): {downgraded_count} ({downgraded_percentage:.2f}%)")
            else:
                print("  (No flow data found)")
            
        except Exception as e:
            print(f"Error analyzing {filename}: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Analyze flow level distribution in NS-2 trace files.")
    parser.add_argument("directory", type=str, help="Path to the directory containing trace files")
    parser.add_argument("--files", type=str, help="Comma-separated list of specific filenames to analyze (optional)", default=None)
    
    args = parser.parse_args()
    
    specific_files = None
    if args.files:
        specific_files = [f.strip() for f in args.files.split(",")]
        
    analyze_trace_levels(args.directory, specific_files)
