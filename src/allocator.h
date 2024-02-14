/**
 * @file allocator.h
 * @brief A source file providing a customized malloc-based memory allocator.
 * 
 * @author SSRVodka
 * @date   Feb 12, 2024
 */

#pragma once

#include <CubismFramework.hpp>
#include <ICubismAllocator.hpp>

/**
 * @class Allocator
 * @brief Class that implements memory allocation.
 *
 * Implementation of interface for memory allocation/deallocation process.
 * Called by the framework.
 * 
 * @see Csm::ICubismAllocator
 *
 */
class Allocator : public Csm::ICubismAllocator
{
    /**
    * @brief Allocates memory space.
    *
    * @param[in] size   Size to be allocated.
    * @return           Specified memory area (void*).
    */
    void* Allocate(const Csm::csmSizeType size);

    /**
    * @brief Releases memory space.
    *
    * @param[in] memory Memory to be released.
    */
    void Deallocate(void* memory);

    /**
    * @brief Allocates memory space with specific alignment rule.
    *
    * @param[in] size       Size to be allocated.
    * @param[in] alignment  Alignment rule.
    * @return               alignedAddress
    */
    void* AllocateAligned(const Csm::csmSizeType size, const Csm::csmUint32 alignment);

    /**
    * @brief Releases memory space with specific alignment rule.
    *
    * @param[in] alignedMemory  Memory to be released.
    */
    void DeallocateAligned(void* alignedMemory);
};
