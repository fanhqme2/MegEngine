# -*- coding: utf-8 -*-
from functools import lru_cache
from typing import Iterable, Optional, Sequence, Tuple, Union

import numpy as np

from ..core._imperative_rt import CompNode
from ..core._imperative_rt.core2 import (
    Const,
    apply,
    broadcast_cpp,
    dtype_promotion,
    expand_dims_cpp,
    split_cpp,
    squeeze_cpp,
)
from ..core._wrap import as_device
from ..core.ops import builtin
from ..core.ops.builtin import Copy, Identity
from ..core.tensor.utils import astensor1d, convert_inputs, get_device, subgraph_fn
from ..device import get_default_device
from ..tensor import Tensor
from .elemwise import ceil

__all__ = [
    "arange",
    "broadcast_to",
    "concat",
    "cond_take",
    "cumsum",
    "diag",
    "expand_dims",
    "eye",
    "flatten",
    "full",
    "full_like",
    "gather",
    "linspace",
    "ones",
    "ones_like",
    "repeat",
    "reshape",
    "roll",
    "split",
    "squeeze",
    "stack",
    "scatter",
    "tile",
    "copy",
    "transpose",
    "where",
    "zeros",
    "zeros_like",
]


def diag(inp, k=0) -> Tensor:
    r"""If ``inp`` is a 1D tensor, then returns a 2D tensor with the elements of ``inp`` as the diagonal.
    If ``inp`` is a 2D tensor, then returns a 1D tensor with the diagonal elements of ``inp``.

    Args:
        inp: input tensor.
        k: diagonal in consider. Use :math:`k=0` for the main diagonal, :math:`k>0` for diagonals above the
           main diagonal, and :math:`k<0` for diagonals below the main diagonal. Default: 0.

    Returns:
        the extracted diagonal or constructed diagonal array.

    Examples:
        >>> inp = F.arange(6, dtype='int32').reshape(2,3)
        >>> out = F.diag(inp, k=1)
        >>> out
        Tensor([1 5], dtype=int32, device=xpux:0)
        >>> F.diag(out)
        Tensor([[1 0]
         [0 5]], dtype=int32, device=xpux:0)
    """
    op = builtin.Diag(k=k)
    (result,) = apply(op, inp)
    return result


def eye(N, M=None, *, dtype="float32", device: Optional[CompNode] = None) -> Tensor:
    r"""Returns a 2D tensor with ones on the diagonal and zeros elsewhere.

    Args:
        N: an integer defining the number of rows.
        M: an integer defining the number of columns. If ``M`` is not specified, the number of columns is ``N``. Default: ``None``.
        dtype: the desired data type of the output tensor. Default: ``float32``.
        device: the desired device of the output tensor. Default: if ``None``,
            use the default device (see :func:`~.megengine.get_default_device`).

    Returns:
        eye matrix.

    Examples:
        >>> import numpy as np
        >>> out = F.eye(4, 6, dtype=np.float32)
        >>> out.numpy()
        array([[1., 0., 0., 0., 0., 0.],
               [0., 1., 0., 0., 0., 0.],
               [0., 0., 1., 0., 0., 0.],
               [0., 0., 0., 1., 0., 0.]], dtype=float32)
    """
    if M is not None:
        if isinstance(N, Tensor) or isinstance(M, Tensor):
            shape = astensor1d((N, M))
        else:
            shape = Tensor([N, M], dtype="int32", device=device)
    elif isinstance(N, Tensor):
        shape = N
    else:
        shape = Tensor(N, dtype="int32", device=device)
    op = builtin.Eye(k=0, dtype=dtype, comp_node=device)
    (result,) = apply(op, shape)
    return result


def full(
    shape: Union[int, tuple, list],
    value: Union[bool, int, float, Tensor],
    dtype=None,
    device=None,
) -> Tensor:
    r"""Creates a tensor of shape ``shape`` filled with ``value``.

    Args:
        shape: output tensor shape.
        value: fill value.
        dtype: output tensor data type. If ``dtype`` is ``None``, the output tensor
            data type must be inferred from ``value``. If the value is an ``int``,
            the output tensor data type must be the default integer data type. If the
            value is a ``float``, the output tensor data type must be the default
            floating-point data type. If the value is a ``bool``, the output tensor
            must have boolean data type. Default: ``None``.
        device: device on which to place the created tensor. Default: ``None``.

    Returns:
        a tensor where every element is equal to ``value``.

    Examples:
        >>> import numpy as np
        >>> out = F.full([2,3], 1.5)
        >>> out.numpy()
        array([[1.5, 1.5, 1.5],
               [1.5, 1.5, 1.5]], dtype=float32)
    """

    if isinstance(shape, int):
        shape = (shape,)
    if device is None:
        device = get_default_device()
    x = Const(value, dtype, device)
    if type(shape) in (list, tuple) and len(shape) == 0:
        return x
    return broadcast_to(x, shape)


def ones(
    shape: Union[int, Tuple[int, ...]],
    *,
    dtype="float32",
    device: Optional[CompNode] = None
) -> Tensor:
    r"""Returns a new tensor having a specified shape and filled with ones.

    Args:
        shape (int or sequence of ints): the shape of the output tensor.

    Keyword args:
        dtype (:attr:`.Tensor.dtype`): output tensor data type. Default: ``float32``.
        device (:attr:`.Tensor.device`): device on which to place the created tensor. Default: ``None``.

    Returns:
        a tensor containing ones.

    Examples:
        >>> F.ones(5)
        Tensor([1. 1. 1. 1. 1.], device=xpux:0)
        >>> F.ones((5, ), dtype='int32')
        Tensor([1 1 1 1 1], dtype=int32, device=xpux:0)
        >>> F.ones((2, 2))
        Tensor([[1. 1.]
         [1. 1.]], device=xpux:0)
        >>> F.ones([2, 1])
        Tensor([[1.]
         [1.]], device=xpux:0)
    """
    return full(shape, 1.0, dtype=dtype, device=device)


def zeros(
    shape: Union[int, Tuple[int, ...]],
    *,
    dtype="float32",
    device: Optional[CompNode] = None
) -> Tensor:
    r"""Returns a new tensor having a specified shape and filled with zeros.

    Args:
        shape (int or sequence of ints): the shape of the output tensor.

    Keyword args:
        dtype (:attr:`.Tensor.dtype`): output tensor data type. Default: ``float32``.
        device (:attr:`.Tensor.device`): device on which to place the created tensor. Default: ``None``.

    Returns:
        a tensor containing zeros.

    Examples:
        >>> F.zeros((2, 1))
        Tensor([[0.]
         [0.]], device=xpux:0)
    """
    return full(shape, 0.0, dtype=dtype, device=device)


def zeros_like(inp: Tensor) -> Tensor:
    r"""Returns a tensor filled with zeros with the same shape and data type as input tensor.

    Args:
        inp (Tensor): input tensor.

    Return:
        a tensor containing zeros.

    Examples:
        >>> input = F.arange(9, dtype='int32').reshape(3,3)
        >>> F.zeros_like(input)
        Tensor([[0 0 0]
         [0 0 0]
         [0 0 0]], dtype=int32, device=xpux:0)
    """
    return full_like(inp, 0.0)


def ones_like(inp: Tensor) -> Tensor:
    r"""Returns a tensor filled with ones with the same shape and data type as input tensor.

    Args:
        inp (Tensor): input tensor.

    Return:
        a tensor containing ones.

    Examples:
        >>> input = F.arange(6, dtype='int32').reshape(2,3)
        >>> F.ones_like(input)
        Tensor([[1 1 1]
         [1 1 1]], dtype=int32, device=xpux:0)
    """
    return full_like(inp, 1.0)


def full_like(inp: Tensor, value: Union[int, float]) -> Tensor:
    r"""Returns a tensor filled with given value with the same shape as input tensor.

    Args:
        inp: input tensor.
        value: target value.

    Return:
        output tensor.

    Examples:
        >>> import numpy as np
        >>> inp = Tensor(np.arange(1, 7, dtype=np.int32).reshape(2,3))
        >>> F.full_like(inp, 2)
        Tensor([[2 2 2]
         [2 2 2]], dtype=int32, device=xpux:0)
    """
    x = Const(value, inp.dtype, inp.device)
    if inp.ndim == 0:
        return x

    # set x's format to use FormatTransformation rule for Broadcast.
    rst = broadcast_to(x, inp.shape)
    rst.format = inp.format
    return rst


def broadcast_to(inp: Tensor, shape: Union[int, Iterable[int]]) -> Tensor:
    r"""Broadcasts a tensor to given shape.

    Args:
        inp: input tensor.
        shape: target shape.

    Returns:
        output tensor.

    Examples:
        >>> import numpy as np
        >>> data = Tensor(np.arange(0, 3, dtype=np.float32).reshape(3))
        >>> out = F.broadcast_to(data, (2, 3))
        >>> out.numpy()
        array([[0., 1., 2.],
               [0., 1., 2.]], dtype=float32)
    """
    return broadcast_cpp(inp, shape)


def concat(inps: Iterable[Tensor], axis: int = 0, device=None) -> Tensor:
    r"""Concat some tensors

    Args:
        inps: input tensors to concat.
        axis: over which dimension the tensors are concatenated. Default: 0
        device: which device output will be. Default: None

    Returns:
        output tensor.

    Examples:
        >>> import numpy as np
        >>> data1 = Tensor(np.arange(0, 6, dtype=np.float32).reshape((2, 3)))
        >>> data2 = Tensor(np.arange(6, 12, dtype=np.float32).reshape((2, 3)))
        >>> out = F.concat([data1, data2])
        >>> out.numpy()
        array([[ 0.,  1.,  2.],
               [ 3.,  4.,  5.],
               [ 6.,  7.,  8.],
               [ 9., 10., 11.]], dtype=float32)
    """
    if len(inps) == 1:
        return inps[0]

    if device is None:
        device = get_device(inps)
    device = as_device(device)
    (result,) = apply(builtin.Concat(axis=axis, comp_node=device.to_c()), *inps)
    return result


def stack(inps, axis=0, device=None):
    r"""Concats a sequence of tensors along a new axis.
    The input tensors must have the same shape.

    Args:
        inps: input tensors.
        axis: which axis will be concatenated.
        device: the device output will be. Default: None

    Returns:
        output concatenated tensor.

    Examples:
        >>> import numpy as np
        >>> x1 = Tensor(np.arange(0, 3, dtype=np.float32).reshape((3)))
        >>> x2 = Tensor(np.arange(6, 9, dtype=np.float32).reshape((3)))
        >>> out = F.stack([x1, x2], axis=0)
        >>> out.numpy()
        array([[0., 1., 2.],
               [6., 7., 8.]], dtype=float32)
    """
    if len(inps) > 0 and not isinstance(inps[0].shape, inps[0].__class__):
        shapes = {arr.shape for arr in inps}
        if len(shapes) != 1:
            raise ValueError("All input tensors must have the same shape")

    inps = [expand_dims(inp, axis=axis) for inp in inps]
    return concat(inps, axis=axis, device=device)


def split(inp, nsplits_or_sections, axis=0):
    r"""Splits the input tensor into several smaller tensors.
    When nsplits_or_sections is int, the last tensor may be smaller than others.

    Args:
        inp: input tensor.
        nsplits_or_sections: number of sub tensors or sections information list.
        axis: which axis will be splited.

    Returns:
        output tensor list.

    Examples:
        >>> import os
        >>> import numpy as np
        >>> x = Tensor(np.random.random((10, 20)), dtype=np.float32)
        >>> y = F.split(x, 3)
        >>> z = F.split(x, [6, 17], axis=1)
        >>> print([i.numpy().shape for i in y])
        [(4, 20), (3, 20), (3, 20)]
        >>> print([i.numpy().shape for i in z])
        [(10, 6), (10, 11), (10, 3)]
    """

    return split_cpp(inp, nsplits_or_sections, axis)


def _get_idx(index, axis):
    index_dims = len(index.shape)
    idx = []
    for i in range(index_dims):
        if i != axis:
            shape = [1] * index_dims
            shape[i] = index.shape[i]
            arange = linspace(
                0, index.shape[i] - 1, index.shape[i], device=index.device,
            )
            arange = (
                broadcast_to(arange.reshape(*shape), index.shape)
                .reshape(-1)
                .astype(np.int32)
            )
            idx.append(arange)
        else:
            idx.append(index.reshape(-1))
    return tuple(idx)


def gather(inp: Tensor, axis: int, index: Tensor) -> Tensor:
    # TODO: rewrite doc
    r"""
    Gathers data from input tensor on axis using index.

    For a 3-D tensor, the output is specified by:

    .. code-block::

       out[i][j][k] = inp[index[i][j][k]][j][k] # if axis == 0
       out[i][j][k] = inp[i][index[i][j][k]][k] # if axis == 1
       out[i][j][k] = inp[i][j][index[i][j][k]] # if axis == 2

    if input tensor is a n-dimensional tensor with size
    :math:`(x_0,x_1,...,x_{i-1},x_i,x_{i+1},...,x_{n-1})` and axis=i,
    then index must be a n-dimensional tensor with size
    :math:`(x_0,x_1,...,x_{i-1},y,x_{i+1},...,x_{n-1})` where :math:`y\ge 1` and
    output will have the same size as index.

    Args:
        inp: input tensor.
        axis: along which axis to index.
        index: indices of elements to gather.

    Return:
        output tensor.

    Examples:
        >>> inp = Tensor([
        ...     [1,2], [3,4], [5,6],
        ... ])
        >>> index = Tensor([[0,2], [1,0]])
        >>> F.gather(inp, 0, index)
        Tensor([[1 6]
         [3 2]], dtype=int32, device=xpux:0)
    """
    input_shape = inp.shape
    index_shape = index.shape
    input_dims = len(input_shape)
    index_dims = len(index_shape)
    if input_dims != index_dims:
        raise ValueError(
            "The index tensor must have same dimensions as input tensor, "
            "But the input dims:{}, the index dims:{}".format(input_dims, index_dims)
        )

    if axis < 0 or axis >= input_dims:
        raise ValueError(
            "Index axis {} is output of bounds, should in range [0 {})".format(
                axis, input_dims
            )
        )

    for i in range(input_dims):
        if i != axis and input_shape[i] != index_shape[i]:
            raise ValueError(
                "The input {} and index {} must have the same size apart from axis {}".format(
                    input_shape, index_shape, axis
                )
            )

    idx = _get_idx(index, axis)
    return inp[idx].reshape(index.shape)  # pylint: disable=no-member


def scatter(inp: Tensor, axis: int, index: Tensor, source: Tensor) -> Tensor:
    # TODO: rewrite doc
    r"""
    Writes all values from the tensor source into input tensor
    at the indices specified in the index tensor.

    For each value in source, its output index is specified by its index
    in source for ``axis != dimension`` and by the corresponding value in
    index for ``axis = dimension``.

    For a 3-D tensor, input tensor is updated as:

    .. code-block::

       inp[index[i][j][k]][j][k] = source[i][j][k]  # if axis == 0
       inp[i][index[i][j][k]][k] = source[i][j][k]  # if axis == 1
       inp[i][j][index[i][j][k]] = source[i][j][k]  # if axis == 2

    ``inp``, ``index`` and ``source`` should have same number of dimensions.

    It is also required that ``source.shape(d) <= inp.shape(d)`` and ``index.shape(d) == source.shape(d)``
    for all dimensions ``d``.

    Moreover, the values of index must be between ``0`` and ``inp.shape(axis) - 1`` inclusive.

    Note:
        Please notice that, due to performance issues, the result is uncertain on the GPU device
        if scattering different positions from source to the same destination position
        regard to index tensor.

        Check the following examples, the oup[0][2] is maybe
        from source[0][2] which value is 0.2256 or source[1][2] which value is 0.5339
        if set the index[1][2] from 1 to 0.

    Args:
        inp: inp tensor which to be scattered.
        axis: axis along which to index.
        index: indices of elements to scatter.
        source: source element(s) to scatter.

    Return:
        output tensor.

    Examples:
        >>> import numpy as np
        >>> inp = Tensor(np.zeros(shape=(3,5),dtype=np.float32))
        >>> source = Tensor([[0.9935,0.9465,0.2256,0.8926,0.4396],[0.7723,0.0718,0.5939,0.357,0.4576]])
        >>> index = Tensor([[0,2,0,2,1],[2,0,1,1,2]])
        >>> oup = F.scatter(inp, 0, index,source)
        >>> oup.numpy()
        array([[0.9935, 0.0718, 0.2256, 0.    , 0.    ],
               [0.    , 0.    , 0.5939, 0.357 , 0.4396],
               [0.7723, 0.9465, 0.    , 0.8926, 0.4576]], dtype=float32)
    """
    input_shape = inp.shape
    index_shape = index.shape
    source_shape = source.shape
    input_dims = len(input_shape)
    index_dims = len(index_shape)
    source_dims = len(source_shape)

    if input_dims != index_dims or input_dims != source_dims:
        raise ValueError("The input, source and index tensor must have same dimensions")

    if axis < 0 or axis >= input_dims:
        raise ValueError(
            "Index axis {} is output of bounds, should in range [0 {})".format(
                axis, input_dims
            )
        )

    for i in range(source_dims):
        if source_shape[i] > input_shape[i]:
            raise ValueError(
                "The each shape size for source {} must be less than or equal to input {} ".format(
                    source_shape, input_shape
                )
            )

    for i in range(index_dims):
        if index_shape[i] != source_shape[i]:
            raise ValueError(
                "The each shape size for index {} must be equal to source {} ".format(
                    index_shape, source_shape
                )
            )

    for i in range(index_dims):
        if i != axis and index_shape[i] > input_shape[i]:
            raise ValueError(
                "The index {} must be less than or equal to input {} size apart from axis {}".format(
                    index_shape, input_shape, axis
                )
            )

    idx = _get_idx(index, axis)
    inp[idx] = source.flatten()
    return inp


@lru_cache(maxsize=None)
def _get_where_op(dtype=None, device=None):
    @subgraph_fn(
        "Where",
        dtype=dtype,
        device=device,
        nr_inputs=3,
        jit_fusion=True,
        custom_grad=True,
    )
    def where(inputs, f, c):
        (mask, x, y) = inputs[0:3]
        oup = f("switch_gt0", mask, x)
        ksam = f("-", c(1), mask)
        oup = f("+", oup, f("switch_gt0", ksam, y))
        (oup_grad,) = yield (oup,)
        x_grad = f("switch_gt0", mask, oup_grad)
        y_grad = f("switch_gt0", ksam, oup_grad)
        yield (None, x_grad, y_grad)

    return where


def where(mask: Tensor, x: Tensor, y: Tensor) -> Tensor:
    r"""Selects elements either from Tensor x or Tensor y, according to mask.

    .. math::

        \textrm{out}_i = x_i \textrm{ if } \textrm{mask}_i \textrm{ is True else } y_i

    Args:
        mask: a mask used for choosing ``x`` or ``y``.
        x: first choice.
        y: second choice.

    Returns:
        output tensor.

    Examples:
        >>> import numpy as np
        >>> mask = Tensor(np.array([[True, False], [False, True]], dtype=np.bool))
        >>> x = Tensor(np.array([[1, np.inf], [np.nan, 4]],
        ...     dtype=np.float32))
        >>> y = Tensor(np.array([[5, 6], [7, 8]], dtype=np.float32))
        >>> out = F.where(mask, x, y)
        >>> out.numpy()
        array([[1., 6.],
               [7., 4.]], dtype=float32)
    """

    if not isinstance(x, Tensor):
        raise TypeError("input x must be a tensor")
    if not isinstance(y, Tensor):
        raise TypeError("input y must be a tensor")
    if not isinstance(mask, Tensor):
        raise TypeError("mask must be a tensor")
    if mask.dtype != np.bool_:
        raise ValueError("mask must be bool")
    if x.device != mask.device:
        raise ValueError("ambiguous device: {} vs {}".format(x.device, mask.device))

    dtype = dtype_promotion(x, y)
    device = x.device

    if x.dtype != dtype:
        x = x.astype(dtype)
    if y.dtype != dtype:
        y = y.astype(dtype)
    mask = mask.astype(dtype)

    where = _get_where_op(dtype=dtype, device=device)
    (oup,) = where(mask, x, y)
    return oup


def cond_take(mask: Tensor, x: Tensor) -> Tensor:
    r"""Takes elements from data if specific condition is satisfied on mask.
    This operator has two outputs: the first is the elements taken,
    and the second is the indices corresponding to those elements;
    they are both 1-dimensional. High-dimension input would first be flattened.

    Args:
        mask: condition param; must be the same shape with data.
        x: input tensor from which to take elements.

    Examples:
        >>> import numpy as np
        >>> mask = Tensor(np.array([[True, False], [False, True]], dtype=np.bool_))
        >>> x = Tensor(np.array([[1, np.inf], [np.nan, 4]],
        ...     dtype=np.float32))
        >>> v, index = F.cond_take(mask, x)
        >>> print(v.numpy(), index.numpy())
        [1. 4.] [0 3]
    """
    if not isinstance(x, Tensor):
        raise TypeError("input must be a tensor")
    if not isinstance(mask, Tensor):
        raise TypeError("mask must be a tensor")
    if mask.dtype != np.bool_:
        raise ValueError("mask must be bool")
    if x.device != mask.device:
        raise ValueError("ambiguous device: {} vs {}".format(x.device, mask.device))

    op = builtin.CondTake()
    v, index = apply(op, x, mask)
    return v, index


def transpose(inp: Tensor, pattern: Iterable[int]) -> Tensor:
    r"""Swaps shapes and strides according to given pattern.

    Args:
        inp: input tensor.
        pattern: a list of integers including 0, 1, ... , ``ndim``-1,
            and any number of ``'x'`` char in dimensions where this tensor should be broadcasted.
            For examples:

            * (``'x'``) -> make a 0d (scalar) into a 1d vector
            * (0, 1) -> identity for 2d vectors
            * (1, 0) -> inverts the first and second dimensions
            * (``'x'``, 0) -> make a row out of a 1d vector (N to 1xN)
            * (0, ``'x'``) -> make a column out of a 1d vector (N to Nx1)
            * (2, 0, 1) -> AxBxC to CxAxB
            * (0, ``'x'``, 1) -> AxB to Ax1xB
            * (1, ``'x'``, 0) -> AxB to Bx1xA
            * (1,) -> this removes dimensions 0. It must be a broadcastable dimension (1xA to A)

    Returns:
        output tensor.

    Examples:
        >>> import numpy as np
        >>> x = Tensor(np.array([[1, 1], [0, 0]], dtype=np.int32))
        >>> F.transpose(x, (1, 0))
        Tensor([[1 0]
         [1 0]], dtype=int32, device=xpux:0)
    """
    return inp.transpose(pattern)


def reshape(inp: Tensor, target_shape: Iterable[int]) -> Tensor:
    r"""Reshapes a tensor without changing its data.

    Args:
        inp: input tensor to reshape.
        target_shape: target shape compatible with the original shape. One shape dimension is allowed 
             to be `-1` . When a shape dimension is `-1` , the corresponding output tensor shape dimension 
             must be inferred from the length of the tensor and the remaining dimensions.

    Returns:
        an output tensor having the same data type, elements, and underlying element order as `inp` .

    Examples:
        >>> x = F.arange(12)
        >>> x
        Tensor([ 0.  1.  2.  3.  4.  5.  6.  7.  8.  9. 10. 11.], device=xpux:0)
        >>> F.reshape(x, (3, 4))
        Tensor([[ 0.  1.  2.  3.]
         [ 4.  5.  6.  7.]
         [ 8.  9. 10. 11.]], device=xpux:0)
        >>> F.reshape(x, (2, -1))
        Tensor([[ 0.  1.  2.  3.  4.  5.]
         [ 6.  7.  8.  9. 10. 11.]], device=xpux:0)
    """
    return inp.reshape(target_shape)


def flatten(inp: Tensor, start_axis: int = 0, end_axis: int = -1) -> Tensor:
    r"""Reshapes the tensor by flattening the sub-tensor from dimension ``start_axis`` to dimension ``end_axis``.

    Args:
        inp: input tensor.
        start_axis: start dimension that the sub-tensor to be flattened. Default: 0
        end_axis: end dimension that the sub-tensor to be flattened. Default: -1

    Returns:
        output tensor.

    Examples:
        >>> import numpy as np
        >>> inp_shape = (2, 2, 3, 3)
        >>> x = Tensor(
        ...     np.arange(36, dtype=np.int32).reshape(inp_shape),
        ... )
        >>> out = F.flatten(x, 2)
        >>> x.numpy().shape
        (2, 2, 3, 3)
        >>> out.numpy().shape
        (2, 2, 9)
    """
    target_shape = tuple(inp.shape[i] for i in range(start_axis)) + (-1,)
    if end_axis != -1:
        target_shape += (*inp.shape[end_axis + 1 :],)
    return inp.reshape(*target_shape)


def expand_dims(inp: Tensor, axis: Union[int, Sequence[int]]) -> Tensor:
    r"""Adds dimension before given axis.

    Args:
        inp: input tensor.
        axis: place of new axes.

    Returns:
        output tensor.

    Examples:
        >>> import numpy as np
        >>> x = Tensor([1, 2])
        >>> out = F.expand_dims(x, 0)
        >>> out.numpy().shape
        (1, 2)
    """

    return expand_dims_cpp(inp, axis)


def squeeze(inp: Tensor, axis: Optional[Union[int, Sequence[int]]] = None) -> Tensor:
    r"""Removes dimension of shape 1.

    Args:
        inp: input tensor.
        axis: place of axis to be removed.

    Returns:
        output tensor.

    Examples:
        >>> import numpy as np
        >>> x = Tensor(np.array([1, 2], dtype=np.int32).reshape(1, 1, 2, 1))
        >>> out = F.squeeze(x, 3)
        >>> out.numpy().shape
        (1, 1, 2)
    """
    return squeeze_cpp(inp, axis)


def linspace(
    start: Union[int, float, Tensor],
    stop: Union[int, float, Tensor],
    num: Union[int, Tensor],
    dtype="float32",
    device: Optional[CompNode] = None,
) -> Tensor:
    r"""Returns equally spaced numbers over a specified interval.

    Args:
        start: starting value of the squence, shoule be scalar.
        stop: last value of the squence, shoule be scalar.
        num: number of values to generate.
        dtype: result data type.

    Returns:
        generated tensor.

    Examples:
        >>> import numpy as np
        >>> a = F.linspace(3, 10, 5)
        >>> a.numpy()
        array([ 3.  ,  4.75,  6.5 ,  8.25, 10.  ], dtype=float32)
    """
    for item in (start, stop, num):
        cur_device = getattr(item, "device", None)
        if device is None:
            device = cur_device
        else:
            if not (cur_device is None or device == cur_device):
                raise ("ambiguous device for linspace opr")

    if not isinstance(start, Tensor):
        start = Tensor(start, device=device)
    if not isinstance(stop, Tensor):
        stop = Tensor(stop, device=device)
    if not isinstance(num, Tensor):
        num = Tensor(num, device=device)

    op = builtin.Linspace(comp_node=device)
    (result,) = apply(op, start, stop, num)
    if np.dtype(dtype) != np.float32:
        return result.astype(dtype)
    return result


def arange(
    start: Union[int, float, Tensor] = 0,
    stop: Optional[Union[int, float, Tensor]] = None,
    step: Union[int, float, Tensor] = 1,
    dtype="float32",
    device: Optional[CompNode] = None,
) -> Tensor:
    r"""Returns evenly spaced values within the half-open interval ``[start, stop)`` as a one-dimensional tensor.

    Note:
        This function cannot guarantee that the interval does not include the stop value in those cases
        where step is not an integer and floating-point rounding errors affect the length of the output tensor.

    Args:
        start: if ``stop`` is specified, the start of interval (inclusive); otherwise,
            the end of the interval (exclusive). If ``stop`` is not specified, the default starting value is ``0``.
        stop: the end of the interval. Default: ``None``.
        step: the distance between two adjacent elements ( ``out[i+1] - out[i]`` ). Must not be 0 ;
            may be negative, this results i an empty tensor if stop >= start . Default: 1 .

    Keyword args:
        dtype( :attr:`.Tensor.dtype` ): output tensor data type. Default: ``float32``.
        device( :attr:`.Tensor.device` ): device on which to place the created tensor. Default: ``None``.

    Returns:
        A one-dimensional tensor containing evenly spaced values.

        The length of the output tensor must be ``ceil((stop-start)/step)``
        if ``stop - start`` and ``step`` have the same sign, and length 0 otherwise.

    Examples:
        >>> F.arange(5)
        Tensor([0. 1. 2. 3. 4.], device=xpux:0)
        >>> F.arange(1, 4)
        Tensor([1. 2. 3.], device=xpux:0)

    """
    if stop is None:
        start, stop = 0, start

    if not isinstance(start, Tensor):
        start = Tensor(start, dtype="float32")
    if not isinstance(stop, Tensor):
        stop = Tensor(stop, dtype="float32")
    if not isinstance(step, Tensor):
        step = Tensor(step, dtype="float32")

    num = ceil((stop - start) / step)
    stop = start + step * (num - 1)
    result = linspace(start, stop, num, device=device)
    if np.dtype(dtype) != np.float32:
        return result.astype(dtype)
    return result


def repeat(inp: Tensor, repeats: int, axis: Optional[int] = None):
    r"""Repeat elements of an array.

    Args:
        inp: input tensor.
        repeats: the number of repetitions for each element.
        axis: the axis along which to repeat values. By default, use the
            flattened input array, and return a flat output array.

    Returns:
        output tensor.

    Examples:
        >>> import numpy as np
        >>> x = Tensor([[1, 2], [3, 4]], np.int32)
        >>> F.repeat(x, 2, axis=0)
        Tensor([[1 2]
         [1 2]
         [3 4]
         [3 4]], dtype=int32, device=xpux:0)
    """
    if axis is None:
        inp = inp.reshape(-1)  # flatten
        axis = 0
    shape = astensor1d(inp.shape, inp, dtype="int32", device=inp.device)
    # assume inp.ndim is not changed during trace
    max_axis = len(shape) - 1
    assert axis >= 0 and axis <= max_axis
    assert repeats >= 1

    base_shape, bcast_shape, target_shape = [], [], []
    if axis != 0:
        target_shape.append(shape[:axis])
    base_shape.extend([shape[: axis + 1], [1,]])
    bcast_shape.extend([shape[: axis + 1], [repeats,]])
    target_shape.extend(
        [shape[axis] * repeats,]
    )
    if axis + 1 <= max_axis:
        base_shape.append(shape[axis + 1 :])
        bcast_shape.append(shape[axis + 1 :])
        target_shape.append(shape[axis + 1 :])

    base_shape = astensor1d(base_shape)
    bcast_shape = astensor1d(bcast_shape)
    target_shape = astensor1d(target_shape)
    out = broadcast_to(inp.reshape(base_shape), bcast_shape).reshape(target_shape)
    return out


def _tile_one_dim(inp, rep, axis):
    shape = astensor1d(inp.shape, inp, dtype="int32", device=inp.device)
    # assume inp.ndim is not changed during trace
    max_axis = len(shape) - 1

    base_shape, bcast_shape, target_shape = [], [], []

    if axis != 0:
        base_shape.append(shape[:axis])
        bcast_shape.append(shape[:axis])
        target_shape.append(shape[:axis])
    base_shape.extend([[1,], shape[axis:]])
    bcast_shape.extend([rep, shape[axis:]])
    target_shape.append(shape[axis] * rep)
    if axis + 1 <= max_axis:
        target_shape.append(shape[axis + 1 :])

    base_shape = astensor1d(base_shape)
    bcast_shape = astensor1d(bcast_shape)
    target_shape = astensor1d(target_shape)
    out = broadcast_to(inp.reshape(base_shape), bcast_shape).reshape(target_shape)
    return out


def tile(inp: Tensor, reps: Iterable[int]):
    r"""Construct an array by repeating ``inp`` the number of times given by ``reps``. If reps has length d,
    the result will have dimension of ``max(d, inp.ndim)``. It is required that ``d >= inp.dim``. If ``inp.ndim < d``,
    ``inp`` is promoted to be ``d``-dimensional by prepending new axis.

    Args:
        inp: input tensor.
        reps: The number of repetitions of inp along each axis.

    Returns:
        output tensor.


    Examples:
        >>> import numpy as np
        >>> x = Tensor([[1, 2], [3, 4]], np.int32)
        >>> F.tile(x, (2,1))
        Tensor([[1 2]
         [3 4]
         [1 2]
         [3 4]], dtype=int32, device=xpux:0)
    """
    shape = astensor1d(inp.shape, inp, dtype="int32", device=inp.device)
    reps = astensor1d(reps, inp, dtype="int32", device=inp.device)
    l_shape = len(shape)
    l_reps = len(reps)
    assert (
        l_reps >= l_shape
    ), "Number of dimensions of tiled dims can not be smaller than number of dimensions of tensor"

    for i in range(l_shape):
        rep = reps[i + (l_reps - l_shape)]
        inp = _tile_one_dim(inp, rep, i)

    if l_reps > l_shape:
        extra = reps[:-l_shape]
        extra_ones = ones_like(extra)
        base_shape = concat([extra_ones, shape])
        bcast_shape = concat([extra, shape])
        target_shape = concat([extra, shape])
        inp = broadcast_to(inp.reshape(base_shape), bcast_shape).reshape(target_shape)

    return inp


def copy(inp, device=None):
    r"""Copies tensor to another device.

    Args:
        inp: input tensor.
        device: destination device.

    Examples:
        >>> import numpy as np
        >>> import platform
        >>> from megengine.device import get_device_count
        >>> x = Tensor([1, 2, 3], np.int32)
        >>> if 1 == get_device_count("gpu"):
        ...     y = F.copy(x, "cpu1")
        ...     print(y.numpy())
        ... else:
        ...     y = F.copy(x, "xpu1")
        ...     print(y.numpy())
        [1 2 3]
    """
    if device is None:
        return apply(Identity(), inp)[0]
    return apply(Copy(comp_node=as_device(device).to_c()), inp)[0]


def roll(
    inp: Tensor,
    shift: Union[int, Iterable[int]],
    axis: Optional[Union[int, Iterable[int]]] = None,
):
    r"""Roll the tensor along the given axis(or axes). Elements that are shifted
    beyond the last position are re-introduced at the first position.

    Args:
        inp: input tensor.
        shift: the number of places by which the elements of the tensor are
            shifted. If shift is a tuple, axis must be a tuple of the same size,
            and each axis will be rolled by the corresponding shift value.
        axis: axis along which to roll. If axis is not specified, the tensor
            will be flattened before rolling and then restored to the original shape.
            Duplicate axes is allowed if it is a tuple. Default: None.

    Examples:
        >>> import numpy as np
        >>> x = Tensor([[1,2],[3,4],[5,6]], np.int32)
        >>> F.roll(x, 1, 0)
        Tensor([[5 6]
         [1 2]
         [3 4]], dtype=int32, device=xpux:0)
    """
    shp_bak = None
    if axis is None:
        shp_bak = inp.shape
        inp = inp.flatten()
        axis = 0
    shp = inp.shape
    dim = len(shp)
    if isinstance(shift, int):
        assert isinstance(axis, int)
        shift, axis = [shift,], [axis,]
    assert len(shift) == len(axis)
    out = inp
    for i in range(len(shift)):
        axis_ = axis[i]
        shift_ = shift[i]
        axis_normalized_ = axis_ + dim if axis_ < 0 else axis_
        assert (
            dim > axis_normalized_ >= 0
        ), "axis out of range (expected to be in range of [{}, {}], but got {})".format(
            -dim, dim - 1, axis_
        )
        if shift_ == 0:
            continue
        size = shp[axis_normalized_]
        shift_normalized_ = 0 if size == 0 else shift_ % size
        if shift_normalized_ > 0:
            a, b = split(out, [size - shift_normalized_,], axis=axis_normalized_)
        else:
            a, b = split(out, [-shift_normalized_,], axis=axis_normalized_)
        out = concat((b, a), axis=axis_normalized_)
    if shp_bak is not None:
        out = out.reshape(shp_bak)
    return out


def cumsum(inp: Tensor, axis: int):
    r"""Computes the cumulative sum of elements along given axis.

    Args:
        inp: input tensor.
        axis: axis along which cumsum is performed.

    Examples:
        >>> x = Tensor([[1, 2, 3], [4, 5, 6]], "int32")
        >>> F.cumsum(x, 1)
        Tensor([[ 1  3  6]
         [ 4  9 15]], dtype=int32, device=xpux:0)
    """
    assert isinstance(inp, Tensor), "input of cumsum must be type of Tensor"
    assert axis >= 0 and axis < inp.ndim, "input axis {} out of bound".format(axis)
    op = builtin.Cumsum(axis=axis, exclusive=False, reverse=False)
    return apply(op, inp)[0]
