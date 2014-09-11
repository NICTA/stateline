import stateline.mcmc as mcmc
import numpy as np
import shutil


def test_chainarray_new_chain():
    shutil.rmtree('testChainDB')

    chain = mcmc.ChainArray(1, 3, recover=False, db_path='testChainDB')

    assert chain.nstacks == 1
    assert chain.nchains == 3
    assert chain.length(0) == 0
    assert chain.length(1) == 0
    assert chain.length(2) == 0


def test_chainarray_initialise():
    shutil.rmtree('testChainDB')

    chain = mcmc.ChainArray(1, 3, recover=False, db_path='testChainDB')
    chain.initialise(0, [1, 2, 3], 1.0, [1], 666.0)
    chain.initialise(1, [4, 5, 6], 2.0, [2], 777.0)
    chain.initialise(2, [7, 8, 9], 3.0, [3], 888.0)

    assert chain.length(0) == 1
    assert chain.length(1) == 1
    assert chain.length(2) == 1

    assert (chain.last_state(0).sample == np.array([1, 2, 3])).all()
    assert (chain.last_state(1).sample == np.array([4, 5, 6])).all()
    assert (chain.last_state(2).sample == np.array([7, 8, 9])).all()

    assert chain.last_state(0).energy == 1.0
    assert chain.last_state(1).energy == 2.0
    assert chain.last_state(2).energy == 3.0

    assert (chain.last_state(0).sigma == np.array([1])).all()
    assert (chain.last_state(1).sigma == np.array([2])).all()
    assert (chain.last_state(2).sigma == np.array([3])).all()

    assert chain.last_state(0).beta == 666.0
    assert chain.last_state(1).beta == 777.0
    assert chain.last_state(2).beta == 888.0


def test_chainarray_set_sigma():
    shutil.rmtree('testChainDB')

    chain = mcmc.ChainArray(1, 1, recover=False, db_path='testChainDB')
    chain.initialise(0, [1, 2, 3], 1.0, [1], 666.0)

    chain.set_sigma(0, [10])
    chain.append(0, [4, 5, 6], 1.0)

    assert chain.last_state(0).sigma == [10]


def test_chainarray_set_beta():
    shutil.rmtree('testChainDB')

    chain = mcmc.ChainArray(1, 1, recover=False, db_path='testChainDB')
    chain.initialise(0, [1, 2, 3], 1.0, [1], 666.0)

    chain.set_beta(0, 777.0)
    chain.append(0, [4, 5, 6], 1.0)

    assert chain.last_state(0).beta == 777.0
