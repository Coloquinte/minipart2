#!/usr/bin/python3

from collections import namedtuple, defaultdict
import sqlite3
import datetime
import subprocess
import multiprocessing
import pandas as pd

MinipartParams  = namedtuple("MinipartParams", ["bench", "version", "partitions", "imbalance", "objective", "v_cycles", "pool_size", "move_ratio", "seed"])
MinipartResults = namedtuple("MinipartResults", ["cut", "connectivity", "max_degree", "daisy_chain_distance", "daisy_chain_max_degree"])
MinipartData    = namedtuple("MinipartData", MinipartParams._fields + MinipartResults._fields)

def extractParams(data):
    fields = [d for f, d in zip(MinipartParams._fields, data)]
    return MinipartParams(*fields)

def extractResults(data):
    fields = data[len(MinipartParams._fields):]
    return MinipartResults(*fields)

def create_db():
    conn = sqlite3.connect("results.db")
    c = conn.cursor()
    c.execute('''
        CREATE TABLE IF NOT EXISTS minipart 
            (bench text, version text, partitions integer, imbalance real, objective text,
              v_cycles integer, pool_size integer, move_ratio real, seed integer,
              cut integer, connectivity integer, max_degree integer,
              daisy_chain_distance integer, daisy_chain_max_degree integer,
              UNIQUE (bench, version, partitions, imbalance, objective, v_cycles, pool_size, move_ratio, seed)
            )
        ''')
    conn.commit()
    c.close()

def list_benchs():
    return [ "ibm%02d" % (i,) for i in range(1,19)]

def list_partitions():
    return [2, 3, 4]

def list_imbalance():
    return [5]

def list_objective(nb_partitions):
    if nb_partitions > 2:
        return ["cut", "soed", "max-degree", "daisy-chain-distance", "daisy-chain-max-degree"]
        #return ["daisy-chain-distance", "daisy-chain-max-degree"]
    else:
        return ["cut"]
        #return []

def list_v_cycles():
    return [8]

def list_pool_size():
    return [32]

def list_move_ratio():
    return [8]

def list_seeds():
    return [1,2,3,4,5,6]

def list_params():
    params = []
    version = "2019-06-10"
    for seed in list_seeds():
      for partitions in list_partitions():
        for imbalance in list_imbalance():
          for objective in list_objective(partitions):
            for v_cycles in list_v_cycles():
              for pool_size in list_pool_size():
                for move_ratio in list_move_ratio():
                  for bench in list_benchs():
                      cur = MinipartParams(bench, version, partitions, imbalance, objective, v_cycles, pool_size, move_ratio, seed)
                      params.append(cur)
    return params

def get_params_result(params):
    conn = sqlite3.connect("results.db")
    c = conn.cursor()
    c.execute('''SELECT * FROM minipart
      WHERE bench=? and version=? and partitions=? and imbalance=?
      and objective=? and v_cycles=? and pool_size=?
      and move_ratio=? and seed=?''', params)
    res = c.fetchall()
    assert len(res) <= 1
    if len(res) == 0:
      return None
    else:
      return MinipartData(*res[0])

def get_version_results(version):
    conn = sqlite3.connect("results.db")
    c = conn.cursor()
    c.execute('''SELECT * FROM minipart
      WHERE version=?''', (version,) )
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
    # Two partitions
    assert cut is not None
    assert (connectivity is not None) == (max_degree is not None)
    return MinipartResults(cut, connectivity, max_degree, daisy_chain_distance, daisy_chain_max_degree)

def run_benchmark(params):
    output = subprocess.check_output(["./minipart_bench",
        "--input", "data/" + params.bench + ".hgr", "--partitions", str(params.partitions),
        "--imbalance", str(params.imbalance), "--objective", params.objective,
        "--v-cycles", str(params.v_cycles), "--pool-size", str(params.pool_size), "--move-ratio", str(params.move_ratio),
        "--seed", str(params.seed)])
    results = extract_metrics(output)
    save_results(params, results)
    return results

def save_results(params, results):
    conn = sqlite3.connect("results.db")
    c = conn.cursor()
    data = params + results
    assert len(data) == 14
    c.execute('''
        INSERT INTO minipart values (?,?,?,?,?,?,?,?,?,?,?,?,?,?)
        ''', data)
    conn.commit()
    c.close()

def run_benchmarks():
    create_db()
    params = list_params()
    params = filter_new_params(params)
    pool = multiprocessing.Pool(8)
    pool.map(run_benchmark, params)

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
        best = min(getattr(r, metric) for r in res_list)
        worst = max(getattr(r, metric) for r in res_list)
        average = sum(getattr(r, metric) for r in res_list) / len(res_list)
        all_metrics += [best, worst, average]
      df.loc[i] = [getattr(params, c) for c in columns_params] + all_metrics
      i += 1
    return df

def gather_results():
    all_results = get_version_results('2019-05-05')
    res = defaultdict(list)
    for data in all_results:
      params = extractParams(data)
      params = params._replace(seed=None)
      results = extractResults(data)
      res[params].append(results)
    cut_df = extract_dataframe(res, "cut")
    cut_df.to_csv("report_cut.csv", index=False)
    cut_df = extract_dataframe(res, "soed")
    cut_df.to_csv("report_connectivity.csv", index=False)
    cut_df = extract_dataframe(res, "max-degree")
    cut_df.to_csv("report_max-degree.csv", index=False)

run_benchmarks()
#gather_results()

