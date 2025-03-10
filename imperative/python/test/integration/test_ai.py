# -*- coding: utf-8 -*-
import numpy as np

import megengine
import megengine.autodiff as ad
import megengine.optimizer as optimizer
from megengine import Parameter, tensor
from megengine.module import Module


class Simple(Module):
    def __init__(self):
        super().__init__()
        self.a = Parameter([1.0], dtype=np.float32)

    def forward(self, x):
        x = x[:, 0] * self.a
        return x


def test_ai():
    net = Simple()

    gm = ad.GradManager().attach(net.parameters())
    optim = optimizer.SGD(net.parameters(), lr=1.0)
    optim.clear_grad()

    dshape = (10, 10)
    data = tensor(np.ones(dshape).astype(np.float32))
    with gm:
        loss = net(data).sum()
        gm.backward(loss)
    optim.step()
    np.testing.assert_almost_equal(
        net.a.numpy(), np.array([1.0 - dshape[0]]).astype(np.float32)
    )
