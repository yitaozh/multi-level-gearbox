#!/usr/bin/env python3
import argparse
import os
import sys
import math
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from scipy.interpolate import interp1d

# --- Helper Functions ---

def humanbytes(B):
    """Return the given bytes as a human friendly KB, MB, GB, or TB string"""
    B = float(B)
    KB = float(1000)
    MB = float(KB ** 2)
    GB = float(KB ** 3)
    TB = float(KB ** 4)

    if B < KB:
        return '{0} {1}'.format(B,'Bytes' if 0 == B > 1 else 'Byte')
    elif KB <= B < MB:
        return '{0:.2f} KB'.format(B/KB)
    elif MB <= B < GB:
        return '{0:.2f} MB'.format(B/MB)
    elif GB <= B < TB:
        return '{0:.2f} GB'.format(B/GB)
    elif TB <= B:
        return '{0:.2f} TB'.format(B/TB)

def loadNormalizeList(directory, flow_size):
    """
    Load normalization data from trace files.
    Note: Filename pattern is hardcoded based on the original notebook.
    """
    offset = 14 - 1   # Peixuan
    result = []
    
    # Ensure directory ends with /
    if not directory.endswith('/'):
        directory += '/'
        
    for size in flow_size:
        # Construct filename - adjust this pattern if your files are named differently
        filename = directory + 'tcp_AFQ10UlimPL_Topology-4hosts-4pods-random-fix.tcl_CDF_' + str(size) + '.tcl.tr'
        
        try:
            with open(filename, 'r') as f:
                line_raw = f.read()
                line = line_raw.split()
                # The notebook extracted the value at offset
                if len(line) > offset:
                    result.append(float(line[offset]))
                else:
                    # print(f"Warning: File {filename} content too short.")
                    result.append(1.0) # Fallback
        except FileNotFoundError:
            # print(f"Warning: Normalization file {filename} not found. Using 1.0 as placeholder.")
            result.append(1.0) # Fallback to avoid crash if files missing

    return [flow_size, result]

# --- FCT Analysis Class ---

class FCT:
    def __init__(self, *args):
        # args: folder, toponame, flownum, loads, algs, normalize_lists
        if len(args) == 6:
            self.loadFromFolder(args[0], args[1], args[2], args[3], args[4], args[5])
        elif len(args) == 4:
            self.loadFromFileList(args[0], args[1], args[2], args[3])
        else:
            raise Exception('parameter number must be 6 or 4')

    def loadFromFolder(self, folder, toponame, flownum, loads, algs, normalize_lists):
        self.loads = loads
        self.algs = algs
        self.raw_data = [[0 for i in range(len(loads))] for i in range(len(algs))]
        
        if not folder.endswith('/'):
            folder += '/'
            
        normalizor = self.expandNormalizedList(normalize_lists)
        
        for alg_i in range(len(algs)):
            for load_i in range(len(loads)):
                alg = algs[alg_i]
                load = loads[load_i]
                
                # Construct filename pattern from notebook
                # "tcp_flow_"+str(flownum)+"_"+str(load)+"_"+toponame+"_"+alg+".tr"
                filename = "tcp_flow_" + str(flownum) + "_" + str(load) + "_" + toponame + "_" + alg + ".tr"
                
                self.raw_data[alg_i][load_i] = \
                    self.parse_trace(folder, filename, normalizor)

    def loadFromFileList(self, loads, algs, filelist, normalize_lists):
        if (len(loads) * len(algs) != len(filelist)):
            raise Exception('the filelist is not match with loads and algs')
        self.loads = loads
        self.algs = algs
        self.raw_data = [[0 for i in range(len(loads))] for i in range(len(algs))]
        if len(normalize_lists) != 2 or len(normalize_lists[0]) != len(normalize_lists[1]):
            raise Exception('normalize_lists shoule contains two list with same length')
        normalizor = self.expandNormalizedList(normalize_lists)
        index=0
        for alg_i in range(len(algs)):
            for load_i in range(len(loads)):
                self.raw_data[alg_i][load_i] = self.parse_trace("", filelist[index], normalizor)
                index+=1

    def expandNormalizedList(self, normalize_lists):
        # Handle empty lists to avoid errors
        if not normalize_lists or len(normalize_lists[0]) == 0:
            return lambda x: 1.0

        try:
            interpolate_function = interp1d(normalize_lists[0], normalize_lists[1], kind='linear', fill_value="extrapolate")
            return interpolate_function
        except Exception as e:
            print(f"Warning: Interpolation failed ({e}). Using identity.")
            return lambda x: 1.0

    # Normalized FCT = Actual FCT / min FCT
    # min FCT = flow_size / max bandwidth
    def parse_trace(self, directory, filename, normalizor):
        col = ["tmp_pkts", "flow duration", "fin_fid", "rtt times", \
                   "group_id", "Tw_", "Tp_", "Np_", "?1", "?2", "?3", "?4", "?5", "Average rate", "start time"]
        
        filepath = os.path.join(directory, filename)
        if not os.path.exists(filepath):
            print(f"Error: File {filepath} not found.")
            return pd.DataFrame(columns=col + ["Normalized FCT", "weight_class"])

        try:
            df = pd.read_csv(filepath, sep = ' ', header = None)
            # Handle mismatch in columns if necessary
            if len(df.columns) > len(col):
                df = df.iloc[:, :len(col)]
            df.columns = col
            
            # Application of normalizor
            # Note: normalizor is now a function (interp1d)
            
            # Safe lookup wrapper
            def safe_lookup(x):
                try:
                    val = float(x)
                    return float(normalizor(val))
                except:
                    return 1.0

            df["Normalized FCT"] = df["tmp_pkts"].apply(safe_lookup) / df["Average rate"]
            df = df.dropna(subset=["Normalized FCT"])
            
            # —— 新增：按 flowid%3 分类 ——
            df["weight_class"] = df["fin_fid"] % 3
            return df
        except Exception as e:
            print(f"Error parsing {filepath}: {e}")
            return pd.DataFrame(columns=col + ["Normalized FCT", "weight_class"])

    ## Plotting Functions (Modified to save instead of show)

    def plot_one_FCT_mean_mark(self, alg, load, flow_num, isLast=False, ax=None):
        alg_id = self.algs.index(alg)
        load_id = self.loads.index(load)
        df = self.raw_data[alg_id][load_id]
        
        if df.empty:
            return

        # cut -> (, ]
        cate=pd.cut(df["tmp_pkts"], flow_num, labels=flow_num[:-1])
        selected_flow=[]
        corresponding_flow_num=[]
        for label in flow_num[:-1]:
            seg_data=df[cate == label]
            if seg_data.empty:
                continue  # 跳过空分组
            
            # 16 indicate col 'Normalized FCT' (check index or name)
            # In parse_trace: col list has 15 items. "Normalized FCT" is added at end (index 15)
            # "weight_class" is index 16.
            # Let's use column name to be safe
            mean_fct = seg_data["Normalized FCT"].mean()
            selected_flow.append(mean_fct)
            corresponding_flow_num.append(label)
            
        len_range=[i for i in range(len(corresponding_flow_num))]
        
        if ax is None:
            plt.plot(len_range, selected_flow, label=alg, lw = 3)
        else:
            ax.plot(len_range, selected_flow, label=alg, lw = 3)

    #### 各组平均 FCT 按flow_size分组
    def plot_Mean_FCT_fix_load_mark(self, algs, load, flow_num, output_file=None):
        fig, ax = plt.subplots(figsize=(10, 6))
        
        # Get x-axis labels from the first non-empty dataset
        x_labels = []
        
        for alg in algs:
            self.plot_one_FCT_mean_mark(alg, load, flow_num, ax=ax)
            
        # Set labels and ticks (approximate based on flow_num buckets)
        # We need to regenerate the x-axis mapping for display
        # This is a bit tricky since different algs might have different missing buckets
        # For simplicity, we assume buckets are consistent or just use flow_num[:-1]
        
        # Reconstruct x labels
        labels_txt = [humanbytes(i*1500) for i in flow_num[:-1]]
        ax.set_xticks(range(len(labels_txt)))
        ax.set_xticklabels(labels_txt, rotation=45, fontsize=10)
        
        ax.set_yscale('log')
        ax.set_xlabel('Flow Size', fontsize = 14)
        ax.set_ylabel('FCT Slowdown (Normalized)', fontsize = 14)
        ax.set_title(f"Mean FCT Slowdown (Load: {load})", fontsize = 16)
        ax.legend(fontsize = 12)
        ax.grid(True, alpha=0.3)
        
        plt.tight_layout()
        if output_file:
            plt.savefig(output_file)
            print(f"Saved plot to {output_file}")
        else:
            plt.show()
        plt.close()

    def plot_Mean_FCT_flow_vs_weight(self, alg, load, flow_num, output_file=None):
        """
        横轴为 flow size bin
        同一张图画出 weight_class = 0,1,2 三条线
        """
        if alg not in self.algs or load not in self.loads:
            return

        alg_id = self.algs.index(alg)
        load_id = self.loads.index(load)
        df = self.raw_data[alg_id][load_id]

        if df.empty:
            print(f"No data for {alg} at load {load}")
            return

        fig, ax = plt.subplots(figsize=(10, 6))

        # weight class = 0,1,2 分别画线
        for w in [0, 1, 2]:
            df_w = df[df["weight_class"] == w]
            if df_w.empty:
                continue

            # --- 分 flow size 桶 ---
            cate = pd.cut(df_w["tmp_pkts"], flow_num, labels=flow_num[:-1])

            selected_flow=[]
            corresponding_flow_num=[]

            for label in flow_num[:-1]:
                seg = df_w[cate == label]
                if seg.empty:
                    continue

                selected_flow.append(seg["Normalized FCT"].mean())
                corresponding_flow_num.append(label)

            if len(selected_flow) == 0:
                continue

            x_indices = range(len(corresponding_flow_num))
            ax.plot(
                x_indices,
                selected_flow,
                marker='o',
                lw=3,
                label=f"weight={w}"
            )
            
            # Update ticks only once (assuming consistent buckets)
            if w == 0: # or logic to set it at least once
                labels_txt = [humanbytes(i*1500) for i in corresponding_flow_num]
                ax.set_xticks(x_indices)
                ax.set_xticklabels(labels_txt, rotation=45)

        ax.set_xlabel("Flow Size")
        ax.set_ylabel("Mean FCT (Normalized)")
        ax.set_yscale("log")
        ax.set_title(f"{alg} | Load={load} | Weight Analysis")
        ax.legend(fontsize=12)
        ax.grid(True, alpha=0.3)
        
        plt.tight_layout()
        if output_file:
            plt.savefig(output_file)
            print(f"Saved plot to {output_file}")
        else:
            plt.show()
        plt.close()

# --- Main Execution ---

def main():
    parser = argparse.ArgumentParser(description="Analyze FCT results from NS-2 simulations.")
    
    parser.add_argument("--folder", type=str, required=True, help="Path to the folder containing .tr result files")
    parser.add_argument("--topo", type=str, default="Topology-8hosts-NSDI21.tcl", help="Topology filename (e.g. Topology-8hosts-NSDI21.tcl)")
    parser.add_argument("--flow_num", type=int, default=1000, help="Number of flows")
    parser.add_argument("--load", type=float, default=0.9, help="Load factor (e.g. 0.9)")
    parser.add_argument("--algs", type=str, required=True, help="Comma-separated list of algorithms (e.g. 1x100,2x50)")
    parser.add_argument("--output", type=str, default="fct_plot.png", help="Output filename for the plot")
    parser.add_argument("--norm_dir", type=str, default="./flow_benchmark/", help="Directory containing normalization trace files")
    parser.add_argument("--plot_type", type=str, choices=["mean", "weight"], default="mean", help="Type of plot to generate")
    
    args = parser.parse_args()
    
    # Parse algs
    alg_list = [a.strip() for a in args.algs.split(",")]
    loads = [args.load]
    
    # Define flow size buckets (hardcoded as in notebook)
    flow_size = [0, 6, 13, 19, 33, 53, 133, 667, 1333, 3333, 6667, 20000, float('inf')]
    
    # Load normalization lists
    print(f"Loading normalization data from {args.norm_dir}...")
    normalize_lists = loadNormalizeList(args.norm_dir, flow_size[1:-1])
    
    # Initialize Analysis
    print(f"Analyzing data from {args.folder}...")
    try:
        fct_analyzer = FCT(args.folder, args.topo, args.flow_num, loads, alg_list, normalize_lists)
        
        # Generate Plot
        if args.plot_type == "mean":
            print(f"Generating Mean FCT plot...")
            fct_analyzer.plot_Mean_FCT_fix_load_mark(alg_list, args.load, flow_size, output_file=args.output)
        elif args.plot_type == "weight":
            print(f"Generating Weight Analysis plots...")
            # For weight analysis, we generate one plot per algo
            base_name, ext = os.path.splitext(args.output)
            for alg in alg_list:
                out_file = f"{base_name}_{alg}{ext}"
                fct_analyzer.plot_Mean_FCT_flow_vs_weight(alg, args.load, flow_size, output_file=out_file)
                
    except Exception as e:
        print(f"Analysis failed: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()
