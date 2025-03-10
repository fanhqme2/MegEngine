# -*- coding: utf-8 -*-
import subprocess

import numpy as np
import pytest

import megengine
import megengine.autodiff as ad
import megengine.optimizer as optimizer
from megengine import Parameter, tensor
from megengine.module import Module


class Simple(Module):
    def __init__(self):
        super().__init__()
        self.a = Parameter([1.23], dtype=np.float32)

    def forward(self, x):
        x = x * self.a
        return x


def test_hello_world():
    net = Simple()

    optim = optimizer.SGD(net.parameters(), lr=1.0)
    optim.clear_grad()
    gm = ad.GradManager().attach(net.parameters())

    data = tensor([2.34])
    with gm:
        loss = net(data)
        gm.backward(loss)
    optim.step()
    np.testing.assert_almost_equal(
        net.a.numpy(), np.array([1.23 - 2.34]).astype(np.float32)
    )
