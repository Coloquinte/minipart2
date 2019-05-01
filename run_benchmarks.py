#!/usr/bin/python3

from collections import namedtuple
import sqlite3
import datetime
import subprocess
import multiprocessing

MinipartParams  = namedtuple("MinipartParams", ["bench", "version", "partitions", "imbalance", "objective", "v_cycles", "pool_size", "move_ratio", "seed"])
MinipartResults = namedtuple("MinipartParams", ["cut", "connectivity", "max_degree"])
MinipartData    = namedtuple("MinipartData", MinipartParams._fields + MinipartResults._fields)

def create_db():
    conn = sqlite3.connect("results.db")
    c = conn.cursor()
    c.execute('''
        CREATE TABLE IF NOT EXISTS minipart 
            (bench text, version text, partitions integer, imbalance real, objective text,
              v_cycles integer, pool_size integer, move_ratio real, seed integer,
              cut integer, connectivity integer, max_degree integer,
              UNIQUE (bench, version, partitions, imbalance, objective, v_cycles, pool_size, move_ratio, seed)
            )
        ''')
    conn.commit()
    c.close()

def list_benchs():
    return [ "ibm%02d" % (i,) for i in range(1,19)]

def list_partitions():
    return [2, 3]

def list_imbalance():
    return [5]

def list_objective(nb_partitions):
    if nb_partitions > 2:
        return ["cut", "soed", "max-degree"]
    else:
        return ["cut"]

def list_v_cycles():
    return [8]

def list_pool_size():
    return [32]

def list_move_ratio():
    return [16]

def list_seeds():
    return [1,2,3,4,5,6]

def list_params():
    params = []
    version = "2019-04-14"
    for bench in list_benchs():
      for partitions in list_partitions():
        for imbalance in list_imbalance():
          for objective in list_objective(partitions):
            for v_cycles in list_v_cycles():
              for pool_size in list_pool_size():
                for move_ratio in list_move_ratio():
                  for seed in list_seeds():
                    cur = MinipartParams(bench, version, partitions, imbalance, objective, v_cycles, pool_size, move_ratio, seed)
                    params.append(cur)
    return params

def filter_new_params(params):
    conn = sqlite3.connect("results.db")
    c = conn.cursor()
    new_params = []
    for p in params:
      c.execute('''SELECT * FROM minipart
        WHERE bench=? and version=? and partitions=? and imbalance=?
        and objective=? and v_cycles=? and pool_size=?
        and move_ratio=? and seed=?''', p)
      res = c.fetchall()
      if len(res) == 0:
        new_params.append(p)
    return new_params

def extract_metrics(output):
    o = output.decode("utf-8")
    cut = None
    connectivity = None
    max_degree = None
    for l in o.splitlines():
      spl = l.split(":")
      if len(spl) >= 2:
        if spl[0] == "Cut":
          cut = int(spl[1])
        elif spl[0] == "Connectivity":
          connectivity = int(spl[1])
        elif spl[0] == "Maximum degree":
          max_degree= int(spl[1])
    # Two partitions
    if connectivity is None:
      connectivity = cut
    if max_degree is None:
      max_degree = cut
    assert cut is not None
    assert (connectivity is not None) == (max_degree is not None)
    return MinipartResults(cut, connectivity, max_degree)

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
    assert len(data) == 12
    c.execute('''
        INSERT INTO minipart values (?,?,?,?,?,?,?,?,?,?,?,?)
        ''', data)
    conn.commit()
    c.close()

def run_benchmarks():
    create_db()
    params = list_params()
    params = filter_new_params(params)
    pool = multiprocessing.Pool(8)
    pool.map(run_benchmark, params)

run_benchmarks()
