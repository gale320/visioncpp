// This file is part of VisionCpp, a lightweight C++ template library
// for computer vision and image processing.
//
// Copyright (C) 2016 Codeplay Software Limited. All Rights Reserved.
//
// Contact: visioncpp@codeplay.com
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/// \file stencil_with_filter.hpp
/// \brief This file contains the StnNoFilt struct which is used to construct a
/// convolutional operation when the value of the filter is fixed and there is
/// no
/// need to pass it as a parameter.

#ifndef VISIONCPP_INCLUDE_FRAMEWORK_EXPR_TREE_NEIGHBOUR_OPS_STENCIL_WITH_FILTER_HPP_
#define VISIONCPP_INCLUDE_FRAMEWORK_EXPR_TREE_NEIGHBOUR_OPS_STENCIL_WITH_FILTER_HPP_

namespace visioncpp {
namespace internal {
/// \struct StnFilt
/// \brief stencil with filter is used to construct a general convolutional
/// operation for a custom filter. It can be used for NeighbourOP
/// \tparam FilterOP: convolution functor with no filter parameter
/// \tparam Halo_T: The top side size of Halo
/// \tparam Halo_L:. The left side size of Halo
/// \tparam Halo_B: The bottom side size of Halo
/// \tparam Halo_R: The right side size of Halo
/// \tparam LHS is the input node
/// \tparam RHS is the filter2d node
/// \tparam Cols: determines the column size of the output
/// \tparam Rows: determines the row size of the output
/// \tparam LfType: determines the type of the leafNode {Buffer2D, Buffer1D,
/// Host, Image}
/// \tparam LVL: the level of the node in the expression tree
template <typename Conv_OP, size_t Halo_T, size_t Halo_L, size_t Halo_B,
          size_t Halo_R, typename LHS, typename RHS, size_t Cols, size_t Rows,
          size_t LfType, size_t LVL>
struct StnFilt {
 public:
  static constexpr bool has_out = false;
  using OutType = typename Conv_OP::OutType;
  using OPType = Conv_OP;
  using LHSExpr = LHS;
  using RHSExpr = RHS;
  using Type = typename visioncpp::internal::OutputMemory<OutType, LfType, Cols,
                                                          Rows, LVL>::Type;
  static constexpr size_t Level = LVL;
  static constexpr size_t LeafType = Type::LeafType;
  static constexpr size_t Operation_type = Conv_OP::Operation_type;
  static constexpr size_t Halo_Top = Halo_T;
  static constexpr size_t Halo_Butt = Halo_B;
  static constexpr size_t Halo_Left = Halo_L;
  static constexpr size_t Halo_Right = Halo_R;
  static constexpr size_t RThread = Rows;
  static constexpr size_t CThread = Cols;
  static constexpr size_t ND_Category = internal::expr_category::Binary;
  static constexpr bool Stencil_Conds =
      ((RThread != LHS::RThread) || (CThread != LHS::CThread));
  static constexpr bool SubExpressionEvaluationNeeded =
      Stencil_Conds || LHS::SubExpressionEvaluationNeeded ||
      RHS::SubExpressionEvaluationNeeded;

  template <typename TmpLHS, typename TmpRHS>
  using ExprExchange =
      internal::StnFilt<Conv_OP, Halo_T, Halo_L, Halo_B, Halo_R, TmpLHS, TmpRHS,
                        Cols, Rows, LfType, LVL>;

  LHS lhs;
  RHS rhs;
  bool subexpr_execution_reseter;
  StnFilt(LHS lhsArg, RHS rhsArg)
      : lhs(lhsArg), rhs(rhsArg), subexpr_execution_reseter(false) {}

  void reset(bool reset) {
    lhs.reset(reset);
    rhs.reset(reset);
    subexpr_execution_reseter = reset;
  }

  /// sub_expression_evaluation
  /// \brief This function is used to break the expression tree whenever
  /// necessary. The decision for breaking the tree will be determined based on
  /// the static parameter called SubExpressionEvaluationNeeded. When this is
  /// set to true, the sub_expression_evaluation is called recursively from the
  /// root of the tree. Each node based on their parent decision will decide to
  /// launch a kernel for itself. Also, they decide for each of their children
  /// whether or not to launch a kernel separately.
  /// template parameters:
  ///\tparam ForcedToExec : a boolean value representing the decision made by
  /// the parent of this node for launching a kernel.
  /// \tparam LC: is the column size of local memory required by Filter2D and
  /// DownSmplOP
  /// \tparam LR: is the row size of local memory required by Filter2D and
  /// DownSmplOP
  /// \tparam LCT: is the column size of workgroup
  /// \tparam LRT: is the row size of workgroup
  /// \tparam DeviceT: type representing the device
  /// function parameters:
  /// \param dev : the selected device for executing the expression
  /// \return LeafNode
  template <bool ForcedToExec, size_t LC, size_t LR, size_t LCT, size_t LRT,
            typename DeviceT>
  auto inline sub_expression_evaluation(const DeviceT& dev)
      -> decltype(execute_expr<Stencil_Conds, ForcedToExec,
                               ExprExchange<LHS, RHS>, LC, LR, LCT, LRT>(
          lhs.template sub_expression_evaluation<Stencil_Conds, LC, LR, LRT,
                                                 LCT>(dev),
          rhs.template sub_expression_evaluation<Stencil_Conds, LC, LR, LRT,
                                                 LCT>(dev),
          dev)) {
    return execute_expr<Stencil_Conds, ForcedToExec, ExprExchange<LHS, RHS>, LC,
                        LR, LCT, LRT>(
        lhs.template sub_expression_evaluation<Stencil_Conds, LC, LR, LCT, LRT>(
            dev),
        rhs.template sub_expression_evaluation<Stencil_Conds, LC, LR, LCT, LRT>(
            dev),
        dev);
  }
};
}  // internal

/// \brief template deduction for StnFilt class when the memory types of the
/// output and column and row are defined by a user.
template <typename OP, size_t Cols, size_t Rows, size_t LeafType, typename LHS,
          typename RHS>
auto neighbour_operation(LHS lhs, RHS rhs) -> internal::StnFilt<
    internal::LocalBinaryOp<OP, typename LHS::OutType, typename RHS::OutType>,
    RHS::Type::Rows / 2, RHS::Type::Cols / 2, RHS::Type::Rows / 2,
    RHS::Type::Cols / 2, LHS, RHS, Cols, Rows, LeafType,
    1 + internal::tools::StaticIf<(LHS::Level > RHS::Level), LHS,
                                  RHS>::Type::Level> {
  return internal::StnFilt<
      internal::LocalBinaryOp<OP, typename LHS::OutType, typename RHS::OutType>,
      RHS::Type::Rows / 2, RHS::Type::Cols / 2, RHS::Type::Rows / 2,
      RHS::Type::Cols / 2, LHS, RHS, Cols, Rows, LeafType,
      1 + internal::tools::StaticIf<(LHS::Level > RHS::Level), LHS,
                                    RHS>::Type::Level>(lhs, rhs);
}

/// \brief template deduction for StnFilt class when the memory type of the
/// output and column and row are automatically deduced from the input.
template <typename OP, typename LHS, typename RHS>
auto neighbour_operation(LHS lhs, RHS rhs) -> internal::StnFilt<
    internal::LocalBinaryOp<OP, typename LHS::OutType, typename RHS::OutType>,
    RHS::Type::Rows / 2, RHS::Type::Cols / 2, RHS::Type::Rows / 2,
    RHS::Type::Cols / 2, LHS, RHS, LHS::Type::Cols, LHS::Type::Rows,
    LHS::Type::LeafType,
    1 + internal::tools::StaticIf<(LHS::Level > RHS::Level), LHS,
                                  RHS>::Type::Level> {
  return internal::StnFilt<
      internal::LocalBinaryOp<OP, typename LHS::OutType, typename RHS::OutType>,
      RHS::Type::Rows / 2, RHS::Type::Cols / 2, RHS::Type::Rows / 2,
      RHS::Type::Cols / 2, LHS, RHS, LHS::Type::Cols, LHS::Type::Rows,
      LHS::Type::LeafType,
      1 + internal::tools::StaticIf<(LHS::Level > RHS::Level), LHS,
                                    RHS>::Type::Level>(lhs, rhs);
}

/// \brief template deduction for StnFilt class when the memory type of the
/// output; halos; and column and row are defined by a user.
template <typename OP, size_t Halo_T, size_t Halo_L, size_t Halo_B,
          size_t Halo_R, size_t Cols, size_t Rows, size_t LeafType,
          typename LHS, typename RHS>
auto neighbour_operation(LHS lhs, RHS rhs) -> internal::StnFilt<
    internal::LocalBinaryOp<OP, typename LHS::OutType, typename RHS::OutType>,
    Halo_T, Halo_L, Halo_B, Halo_R, LHS, RHS, Cols, Rows, LeafType,
    1 + internal::tools::StaticIf<(LHS::Level > RHS::Level), LHS,
                                  RHS>::Type::Level> {
  return internal::StnFilt<
      internal::LocalBinaryOp<OP, typename LHS::OutType, typename RHS::OutType>,
      Halo_T, Halo_L, Halo_B, Halo_R, LHS, RHS, Cols, Rows, LeafType,
      1 + internal::tools::StaticIf<(LHS::Level > RHS::Level), LHS,
                                    RHS>::Type::Level>(lhs, rhs);
}

/// \brief template deduction for StnFilt class when the memory type of the
/// output and column and row are automatically deduced from the input. However,
/// the halos are defined by user.
template <typename OP, size_t Halo_T, size_t Halo_L, size_t Halo_B,
          size_t Halo_R, typename LHS, typename RHS>
auto neighbour_operation(LHS lhs, RHS rhs) -> internal::StnFilt<
    internal::LocalBinaryOp<OP, typename LHS::OutType, typename RHS::OutType>,
    Halo_T, Halo_L, Halo_B, Halo_R, LHS, RHS, LHS::Type::Cols, LHS::Type::Rows,
    LHS::Type::LeafType,
    1 + internal::tools::StaticIf<(LHS::Level > RHS::Level), LHS,
                                  RHS>::Type::Level> {
  return internal::StnFilt<
      internal::LocalBinaryOp<OP, typename LHS::OutType, typename RHS::OutType>,
      Halo_T, Halo_L, Halo_B, Halo_R, LHS, RHS, LHS::Type::Cols,
      LHS::Type::Rows, LHS::Type::LeafType,
      1 + internal::tools::StaticIf<(LHS::Level > RHS::Level), LHS,
                                    RHS>::Type::Level>(lhs, rhs);
}
}  // visioncpp
#endif  // VISIONCPP_INCLUDE_FRAMEWORK_EXPR_TREE_NEIGHBOUR_OPS_STENCIL_WITH_FILTER_HPP_
