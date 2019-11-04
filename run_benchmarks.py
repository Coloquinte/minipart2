#!/usr/bin/python3

from collections import namedtuple, defaultdict
import sqlite3
import datetime
import subprocess
import multiprocessing
import pandas as pd
import os.path
import argparse

MinipartParams  = namedtuple("MinipartParams", ["bench", "input_file", "output_file", "solver", "blocks", "imbalance", "objective", "v_cycles", "pool_size", "move_ratio", "seed"])
MinipartResults = namedtuple("MinipartResults", ["objective_value", "cut", "connectivity", "max_degree"])
MinipartData    = namedtuple("MinipartData", MinipartParams._fields + MinipartResults._fields)

def extract_params(data):
    fields = [d for f, d in zip(MinipartParams._fields, data)]
    return MinipartParams(*fields)

def extract_results(data):
    fields = data[len(MinipartParams._fields):]
    return MinipartResults(*fields)

def create_db():
    conn = sqlite3.connect("results.db")
    c = conn.cursor()
    c.execute('''
        CREATE TABLE IF NOT EXISTS minipart 
            (bench text, input_file text, output_file text, solver text, blocks integer, imbalance real, objective text,
              v_cycles integer, pool_size integer, move_ratio real, seed integer,
              objective_value integer, cut integer, connectivity integer, max_degree integer,
              UNIQUE (bench, solver, blocks, imbalance, objective, v_cycles, pool_size, move_ratio, seed)
            )
        ''')
    conn.commit()
    c.close()

def get_params_result(params):
    conn = sqlite3.connect("results.db")
    c = conn.cursor()
    c.execute('''SELECT * FROM minipart
      WHERE bench=? and solver=? and blocks=? and imbalance=?
      and objective=? and v_cycles=? and pool_size=?
      and move_ratio=? and seed=?''', params)
    res = c.fetchall()
    assert len(res) <= 1
    if len(res) == 0:
      return None
    else:
      return MinipartData(*res[0])

def get_solver_results(solver):
    conn = sqlite3.connect("results.db")
    c = conn.cursor()
    c.execute('''SELECT * FROM minipart
      WHERE solver=?''', (solver,) )
    res = c.fetchall()
    return [MinipartData(*r) for r in res]

def filter_new_params(params):
    conn = sqlite3.connect("results.db")
    c = conn.cursor()
    new_params = []
    for p in params:
      res = get_params_result(p)
      if res is None:
        new_params.append(p)
    return new_params

def list_seeds(args):
    return list(range(1, args.seeds+1))

def list_benchs(args):
    benchs = []
    for b in os.listdir(args.data):
        path = os.path.join(args.data, b)
        benchs.append((os.path.getsize(path), b))
    benchs.sort()
    return [b[1] for b in benchs]

def bench_to_name(bench):
    return bench.replace(".hgr", "")

def bench_to_input_file(args, bench):
    return os.path.join(args.data, bench)

def list_params_minipart(args):
    if not args.minipart:
      return []
    params = []
    solver = "minipart"
    seed_opts = list_seeds(args)
    for seed in seed_opts:
      imbalance_opts = [0.05,]
      for imbalance in imbalance_opts:
        for v_cycles in [1, ]:
          for pool_size in [32,]:
            for move_ratio in [8.0,]:
              for bench in list_benchs(args):
                blocks_opts = [2, 4]
                for blocks in blocks_opts:
                  objective_opts = ["cut", "soed", "max-degree", "daisy-chain-distance", "daisy-chain-max-degree"] if blocks > 2 else ["cut"]
                  # objective_opts = ["cut", "soed"] if blocks > 2 else ["cut"]
                  for objective in objective_opts:
                    cur = MinipartParams(bench_to_name(bench), bench_to_input_file(args, bench), "",
                                         solver, blocks, imbalance, objective,
                                         v_cycles, pool_size, move_ratio, seed)
                    params.append(cur)
    return params

def list_params_kahypar(args):
    if not args.kahypar:
      return []
    params = []
    solver = "kahypar"
    seed_opts = list_seeds(args)
    v_cycles = 1
    pool_size = None
    move_ratio = None
    for seed in seed_opts:
      blocks_opts = range(2, 5)
      for blocks in blocks_opts:
        imbalance_opts = [0.05,]
        for imbalance in imbalance_opts:
          objective_opts = ["cut", "soed"] if blocks > 2 else ["cut"]
          for objective in objective_opts:
            for bench in list_benchs(args):
              cur = MinipartParams(bench_to_name(bench), bench_to_input_file(args, bench), "",
                                   solver, blocks, imbalance, objective,
                                   v_cycles, pool_size, move_ratio, seed)
              params.append(cur)
    return params

def list_params(args):
    return list_params_kahypar(args) + list_params_minipart(args)

def extract_metrics(output, objective):
    o = output.decode("utf-8")
    cut = None
    connectivity = None
    max_degree = None
    daisy_chain_distance = None
    daisy_chain_max_degree = None
    for l in o.splitlines():
      spl = l.split(":")
      if len(spl) >= 2:
        if spl[0] == "Cut":
          cut = int(spl[1])
        elif spl[0] == "Connectivity":
          connectivity = int(spl[1])
        elif spl[0] == "Maximum degree":
          max_degree = int(spl[1])
        elif spl[0] == "Daisy-chain distance":
          daisy_chain_distance = int(spl[1])
        elif spl[0] == "Daisy-chain maximum degree":
          daisy_chain_max_degree = int(spl[1])
    assert cut is not None
    assert (connectivity is not None) == (max_degree is not None)
    assert daisy_chain_distance is None or (connectivity is not None)
    assert daisy_chain_max_degree is None or (connectivity is not None)
    # Case of 2 partitions
    if connectivity is None:
      connectivity = cut
    if max_degree is None:
      max_degree = cut
    objective_value = cut
    if objective == "soed":
        objective_value = connectivity
    elif objective == "max-degree":
        objective_value = max_degree 
    elif objective == "daisy-chain-distance":
        objective_value = daisy_chain_distance
    elif objective == "daisy-chain-max-degree":
        objective_value = daisy_chain_max_degree 
    elif objective != "cut":
        raise RuntimeError("Unknown objective " + objective)
    return MinipartResults(objective_value, cut, connectivity, max_degree)

def compute_filename(params):
    name = "solutions/output_" + params.bench
    name += "_" + str(params.solver)
    name += "_k" + str(params.blocks)
    name += "_e" + str(params.imbalance)
    name += "_" + params.objective
    if params.v_cycles is not None:
      name += "_vcycles" + str(params.v_cycles)
    if params.pool_size is not None:
      name += "_pool" + str(params.pool_size)
    if params.move_ratio is not None:
      name += "_ratio" + str(params.move_ratio)
    name += "_s" + str(params.seed)
    name += ".sol"
    return name

def benchmark_done(params):
    filename = compute_filename(params) + ".gz"
    return os.path.isfile(filename)

def run_benchmark_minipart(params):
    filename = compute_filename(params)
    output = subprocess.check_output(["./minipart_bench",
        "-i", params.input_file,
        "-k", str(params.blocks),
        "-e", str(100.0 * params.imbalance),
        "-g", params.objective,
        "--v-cycles", str(params.v_cycles),
        "--pool-size", str(params.pool_size),
        "--move-ratio", str(params.move_ratio),
        "-s", str(params.seed),
        "-o", filename + ".gz"])
    # No saving the results, as a solution file is generated

def run_benchmark_kahypar(params):
    if params.objective not in ["cut", "soed"]:
      raise RuntimeError("Kahypar only supports cut and soed objectives")
    obj = "km1" if params.objective == "soed" else "cut"
    refine = "kway_fm_flow_km1" if params.objective == "soed" else "kway_fm_flow"
    filename = compute_filename(params)
    args = ["./kahypar_bench", "-p", "kahypar_config", "-m", "direct",
        "-h", params.input_file, "-k", str(params.blocks),
        "-e", str(params.imbalance), "-o", obj,
        "--r-type", refine,
        "--vcycles", str(params.v_cycles),
        "--seed", str(params.seed),
        "--output-file", filename]
    output = subprocess.check_output(args)
    subprocess.check_output(["gzip", filename])
    # No saving the results, as a solution file is generated

def run_benchmark(params):
    if benchmark_done(params):
      return
    if params.solver == "minipart":
      run_benchmark_minipart(params)
    elif params.solver == "kahypar":
      run_benchmark_kahypar(params)
    else:
      raise RuntimeError("Unknown solver type")

def run_benchmarks(args):
    create_db()
    params = list_params(args)
    pool = multiprocessing.Pool(args.threads)
    pool.map(run_benchmark, params)

def eval_benchmark(params):
    filename = compute_filename(params)
    objective_param = params.objective
    if objective_param == "cut" or objective_param == "soed":
      objective_param = "max-degree"
    output = subprocess.check_output(["./minipart_bench",
        "-i", params.input_file,
        "-k", str(params.blocks),
        "-e", str(100.0 * params.imbalance),
        "-g", objective_param,
        "-f", filename + ".gz",
        "--no-solve"])
    return extract_metrics(output, params.objective)

def save_benchmark(params):
    if benchmark_done(params):
        results = eval_benchmark(params)
        conn = sqlite3.connect("results.db")
        c = conn.cursor()
        data = params + results
        assert len(data) == 15
        c.execute('''
            INSERT OR IGNORE INTO minipart values (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)
            ''', data)
        conn.commit()
        c.close()


def save_results(args):
    if not args.save_db:
      return
    create_db()
    params = list_params(args)
    for param in params:
        save_benchmark(param)

def extract_dataframe(params_to_results, objective):
    columns_params = [p for p in MinipartParams._fields if p not in ["seed", "objective"]]
    columns_results = ["nb_runs",
        "best_objective_value", "worst_objective_value", "average_objective_value",
        "best_cut", "worst_cut", "average_cut",
        "best_connectivity", "worst_connectivity", "average_connectivity",
        "best_max_degree", "worst_max_degree", "average_max_degree",
    ]
    df = pd.DataFrame(columns=columns_params + columns_results)
    i = 0
    for params, res_list in sorted(params_to_results.items()):
      if params.objective != objective:
        continue
      all_metrics = [len(res_list), ]
      for metric in ["objective_value", "cut", "connectivity", "max_degree"]:
        metric_list = list(filter(lambda x: x is not None, [getattr(r, metric) for r in res_list]))
        if len(metric_list) > 0:
          best = min(getattr(r, metric) for r in res_list)
          worst = max(getattr(r, metric) for r in res_list)
          average = sum(getattr(r, metric) for r in res_list) / len(res_list)
        else:
          print(all_metrics)
          print(res_list)
          raise RuntimeError("No metric available for " + metric)
          best = None
          worst = None
          average = None
        all_metrics += [best, worst, average]
      df.loc[i] = [getattr(params, c) for c in columns_params] + all_metrics
      i += 1
    return df

def gather_solver_results(solver):
    all_results = get_solver_results(solver)
    res = defaultdict(list)
    for data in all_results:
      params = extract_params(data)
      params = params._replace(seed=None)
      results = extract_results(data)
      res[params].append(results)
    df = extract_dataframe(res, "cut")
    if df.shape[0] > 0:
        df.to_csv("report_cut_" + solver + ".csv", index=False, float_format="%.2f")
    df = extract_dataframe(res, "soed")
    if df.shape[0] > 0:
        df.to_csv("report_connectivity_" + solver + ".csv", index=False, float_format="%.2f")
    df = extract_dataframe(res, "max-degree")
    if df.shape[0] > 0:
        df.to_csv("report_max_degree_" + solver + ".csv", index=False, float_format="%.2f")
    df = extract_dataframe(res, "daisy-chain-distance")
    if df.shape[0] > 0:
        df.to_csv("report_distance_" + solver + ".csv", index=False, float_format="%.2f")
    df = extract_dataframe(res, "daisy-chain-max-degree")
    if df.shape[0] > 0:
        df.to_csv("report_routed_degree_" + solver + ".csv", index=False, float_format="%.2f")

def gather_results(args):
    if args.gather_results:
        if args.kahypar:
            gather_solver_results("kahypar")
        if args.minipart:
            gather_solver_results("minipart")

parser = argparse.ArgumentParser()
parser.add_argument("--data", help="Directory for the hypergraphs", default="data")
#parser.add_argument("--solutions", help="Directory for the solutions", default="solutions")
parser.add_argument("--seeds", help="Number of random seeds to test", default=2, type=int)
parser.add_argument("--threads", help="Number of threads", default=24, type=int)
parser.add_argument("--minipart", help="Run Minipart", action="store_true")
parser.add_argument("--kahypar", help="Run Kahypar", action="store_true")
parser.add_argument("--save_db", help="Save results to database", action="store_true")
parser.add_argument("--gather_results", help="Save results as CSV", action="store_true")
args = parser.parse_args()

run_benchmarks(args)
save_results(args)
gather_results(args)

