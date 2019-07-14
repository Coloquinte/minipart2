#!/usr/bin/python3

from collections import namedtuple, defaultdict
import sqlite3
import datetime
import subprocess
import multiprocessing
import pandas as pd
import os.path

MinipartParams  = namedtuple("MinipartParams", ["bench", "solver", "blocks", "imbalance", "objective", "v_cycles", "pool_size", "move_ratio", "seed"])
MinipartResults = namedtuple("MinipartResults", ["cut", "connectivity", "max_degree", "daisy_chain_distance", "daisy_chain_max_degree"])
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
            (bench text, solver text, blocks integer, imbalance real, objective text,
              v_cycles integer, pool_size integer, move_ratio real, seed integer,
              cut integer, connectivity integer, max_degree integer,
              daisy_chain_distance integer, daisy_chain_max_degree integer,
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

def list_benchs():
    return [ "ibm%02d" % (i,) for i in range(1,19)]

def list_params_minipart():
    params = []
    solver = "minipart"
    seed_opts = range(1, 7)
    for seed in seed_opts:
      blocks_opts = range(2, 5)
      for blocks in blocks_opts:
        imbalance_opts = [0.05,]
        for imbalance in imbalance_opts:
          objective_opts = ["cut", "soed"] if blocks > 2 else ["cut"]
          #objective_opts = ["cut", "soed", "max-degree", "daisy-chain-distance", "daisy-chain-max-degree"]
          for objective in objective_opts:
            for v_cycles in [1, ]:
              for pool_size in [32,]:
                for move_ratio in [8.0,]:
                  for bench in list_benchs():
                    cur = MinipartParams(bench, solver, blocks, imbalance, objective, v_cycles, pool_size, move_ratio, seed)
                    params.append(cur)
    return params

def list_params_kahypar():
    params = []
    solver = "kahypar"
    seed_opts = range(1, 7)
    v_cycles = 1
    for seed in seed_opts:
      blocks_opts = range(2, 5)
      for blocks in blocks_opts:
        imbalance_opts = [0.05,]
        for imbalance in imbalance_opts:
          objective_opts = ["cut", "soed"] if blocks > 2 else ["cut"]
          #objective_opts = ["cut", "soed", "max-degree", "daisy-chain-distance", "daisy-chain-max-degree"]
          for objective in objective_opts:
            for bench in list_benchs():
              cur = MinipartParams(bench, solver, blocks, imbalance, objective, 1, None, None, seed)
              params.append(cur)
    return params

def list_params():
    return list_params_kahypar() + list_params_minipart()

def extract_metrics(output):
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
    return MinipartResults(cut, connectivity, max_degree, daisy_chain_distance, daisy_chain_max_degree)

def compute_filename(params):
    name = "solutions/output_"  + params.bench
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
        "-i", "data/" + params.bench + ".hgr",
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
        "-h", "data/" + params.bench + ".hgr", "-k", str(params.blocks),
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

def run_benchmarks():
    create_db()
    params = list_params()
    pool = multiprocessing.Pool(8)
    pool.map(run_benchmark, params)

def eval_benchmark(params):
    filename = compute_filename(params)
    output = subprocess.check_output(["./minipart_bench",
        "-i", "data/" + params.bench + ".hgr",
        "-k", str(params.blocks),
        "-e", str(100.0 * params.imbalance),
        "-g", "max-degree",
        "-f", filename + ".gz",
        "--no-solve"])
    return extract_metrics(output)

def save_benchmark(params):
    if benchmark_done(params):
      results = eval_benchmark(params)
      save_results(params, results)

def save_benchmarks():
    create_db()
    params = list_params()
    pool = multiprocessing.Pool(8)
    pool.map(save_benchmark, params)

def save_results(params, results):
    conn = sqlite3.connect("results.db")
    c = conn.cursor()
    data = params + results
    assert len(data) == 14
    c.execute('''
        INSERT OR IGNORE INTO minipart values (?,?,?,?,?,?,?,?,?,?,?,?,?,?)
        ''', data)
    conn.commit()
    c.close()

def extract_dataframe(params_to_results, objective):
    columns_params = [p for p in MinipartParams._fields if p not in ["seed", "objective"]]
    columns_results = ["nb_runs",
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
      for metric in ["cut", "connectivity", "max_degree"]:
        metric_list = list(filter(lambda x: x is not None, [getattr(r, metric) for r in res_list]))
        if len(metric_list) > 0:
          best = min(getattr(r, metric) for r in res_list)
          worst = max(getattr(r, metric) for r in res_list)
          average = sum(getattr(r, metric) for r in res_list) / len(res_list)
        else:
          best = None
          worst = None
          average = None
        all_metrics += [best, worst, average]
      df.loc[i] = [getattr(params, c) for c in columns_params] + all_metrics
      i += 1
    return df

def gather_results(solver):
    all_results = get_solver_results(solver)
    res = defaultdict(list)
    for data in all_results:
      params = extract_params(data)
      params = params._replace(seed=None)
      results = extract_results(data)
      res[params].append(results)
    cut_df = extract_dataframe(res, "cut")
    cut_df.to_csv("report_cut_" + solver + ".csv", index=False)
    cut_df = extract_dataframe(res, "soed")
    cut_df.to_csv("report_connectivity_" + solver + ".csv", index=False)
    cut_df = extract_dataframe(res, "max-degree")

#save_benchmarks()
run_benchmarks()
#gather_results("kahypar")
#gather_results("minipart")

