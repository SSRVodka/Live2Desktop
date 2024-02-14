/**
 * @file modelParameters.h
 * @brief Parameters for the model.
 * 
 * @author Copyright(c) Live2D Inc. && SJTU-XHW
 * @date   Feb 12, 2024
 */

#pragma once

#include <CubismFramework.hpp>


namespace ModelParameters {

    using namespace Csm;

    extern const csmFloat32 ViewScale;              /**< Scaling factor */
    extern const csmFloat32 ViewMaxScale;           /**< Maximum of scaling factor */
    extern const csmFloat32 ViewMinScale;           /**< Minimum of scaling factor */

    extern const csmFloat32 ViewLogicalLeft;        /**< Leftmost value of logical view coordinate system */
    extern const csmFloat32 ViewLogicalRight;       /**< Rightmost value of logical view coordinate system */
    extern const csmFloat32 ViewLogicalBottom;      /**< Value of the lower end of the logical view coordinate system */
    extern const csmFloat32 ViewLogicalTop;         /**< Value of the upper end of the logical view coordinate system */

    extern const csmFloat32 ViewLogicalMaxLeft;     /**< Maximum left edge of logical view coordinate system */
    extern const csmFloat32 ViewLogicalMaxRight;    /**< Maximum right edge of logical view coordinate system */
    extern const csmFloat32 ViewLogicalMaxBottom;   /**< Maximum value of the lower edge of the logical view coordinate system */
    extern const csmFloat32 ViewLogicalMaxTop;      /**< Maximum value of the upper edge of the logical view coordinate system */

    extern const csmChar* ResourcesPath;

    /* Match with external definition file (json). */

    extern const csmChar* MotionGroupIdle;          /**< List of motions to play when idling */
    extern const csmChar* MotionGroupTapBody;       /**< List of motions to play when you tap the body */

    extern const csmChar* HitAreaNameHead;          /**< [Head] tag of the hit decision */
    extern const csmChar* HitAreaNameBody;          /**< [Body] tag of the hit decision */

    /* Motion priority constants. */

    extern const csmInt32 PriorityNone;     /**< 0 */
    extern const csmInt32 PriorityIdle;     /**< 1 */
    extern const csmInt32 PriorityNormal;   /**< 2 */
    extern const csmInt32 PriorityForce;    /**< 3 */

    /* Default render target size. */
    extern const csmInt32 RenderTargetWidth;
    extern const csmInt32 RenderTargetHeight;
};
