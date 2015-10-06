/*
 * Copyright 1993-2012 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

#include "bodysystemcpu.h"

#include <assert.h>
#include <memory.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <helper_cuda.h>
#include <algorithm>
#include "tipsy.h"

#ifdef OPENMP
#include <omp.h>
#endif
#include "homp.h"

template <typename T>
BodySystemCPU<T>::BodySystemCPU(int numBodies)
    : m_numBodies(numBodies),
      m_bInitialized(false),
      m_force(0),
      m_softeningSquared(.00125f),
      m_damping(0.995f)
{
    m_pos = 0;
    m_vel = 0;

    _initialize(numBodies);
}

template <typename T>
BodySystemCPU<T>::~BodySystemCPU()
{
    _finalize();
    m_numBodies = 0;
}

template <typename T>
void BodySystemCPU<T>::_initialize(int numBodies)
{
    assert(!m_bInitialized);

    m_numBodies = numBodies;

    m_pos    = new T[m_numBodies*4];
    m_vel    = new T[m_numBodies*4];
    m_force  = new T[m_numBodies*3];

    memset(m_pos,   0, m_numBodies*4*sizeof(T));
    memset(m_vel,   0, m_numBodies*4*sizeof(T));
    memset(m_force, 0, m_numBodies*3*sizeof(T));

    m_bInitialized = true;
}

template <typename T>
void BodySystemCPU<T>::_finalize()
{
    assert(m_bInitialized);

    delete [] m_pos;
    delete [] m_vel;
    delete [] m_force;

    m_bInitialized = false;
}

template <typename T>
void BodySystemCPU<T>::loadTipsyFile(const std::string &filename)
{
    if (m_bInitialized)
        _finalize();

    vector< typename vec4<T>::Type > positions;
    vector< typename vec4<T>::Type > velocities;
    vector< int> ids;

    int nBodies = 0;
    int nFirst=0, nSecond=0, nThird=0;

    read_tipsy_file(positions,
                    velocities,
                    ids,
                    filename,
                    nBodies,
                    nFirst,
                    nSecond,
                    nThird);

    _initialize(nBodies);

    memcpy(m_pos, &positions[0], sizeof(vec4<T>)*nBodies);
    memcpy(m_vel, &velocities[0], sizeof(vec4<T>)*nBodies);
}

template <typename T>
void BodySystemCPU<T>::update(T deltaTime)
{
    assert(m_bInitialized);

    _integrateNBodySystem(deltaTime);

    //std::swap(m_currentRead, m_currentWrite);
}

template <typename T>
T *BodySystemCPU<T>::getArray(BodyArray array)
{
    assert(m_bInitialized);

    T *data = 0;

    switch (array)
    {
        default:
        case BODYSYSTEM_POSITION:
            data = m_pos;
            break;

        case BODYSYSTEM_VELOCITY:
            data = m_vel;
            break;
    }

    return data;
}

template <typename T>
void BodySystemCPU<T>::setArray(BodyArray array, const T *data)
{
    assert(m_bInitialized);

    T *target = 0;

    switch (array)
    {
        default:
        case BODYSYSTEM_POSITION:
            target = m_pos;
            break;

        case BODYSYSTEM_VELOCITY:
            target = m_vel;
            break;
    }

    memcpy(target, data, m_numBodies*4*sizeof(T));
}

template<typename T>
T sqrt_T(T x)
{
    return sqrt(x);
}

template<>
float sqrt_T<float>(float x)
{
    return sqrtf(x);
}

template <typename T>
void bodyBodyInteraction(T accel[3], T posMass0[4], T posMass1[4], T softeningSquared)
{
    T r[3];

    // r_01  [3 FLOPS]
    r[0] = posMass1[0] - posMass0[0];
    r[1] = posMass1[1] - posMass0[1];
    r[2] = posMass1[2] - posMass0[2];

    // d^2 + e^2 [6 FLOPS]
    T distSqr = r[0] * r[0] + r[1] * r[1] + r[2] * r[2];
    distSqr += softeningSquared;

    // invDistCube =1/distSqr^(3/2)  [4 FLOPS (2 mul, 1 sqrt, 1 inv)]
    T invDist = (T)1.0 / (T)sqrt((double)distSqr);
    T invDistCube =  invDist * invDist * invDist;

    // s = m_j * invDistCube [1 FLOP]
    T s = posMass1[3] * invDistCube;

    // (m_1 * r_01) / (d^2 + e^2)^(3/2)  [6 FLOPS]
    accel[0] += r[0] * s;
    accel[1] += r[1] * s;
    accel[2] += r[2] * s;
}

template <typename T>
void BodySystemCPU<T>::_computeNBodyGravitation()
{
#ifdef OPENMP
    #pragma omp parallel for
#endif

    for (int i = 0; i < m_numBodies; i++)
    {
        int indexForce = 3*i;

        T acc[3] = {0, 0, 0};

        // We unroll this loop 4X for a small performance boost.
        int j = 0;

        while (j < m_numBodies)
        {
            bodyBodyInteraction<T>(acc, &m_pos[4*i], &m_pos[4*j], m_softeningSquared);
            j++;
            bodyBodyInteraction<T>(acc, &m_pos[4*i], &m_pos[4*j], m_softeningSquared);
            j++;
            bodyBodyInteraction<T>(acc, &m_pos[4*i], &m_pos[4*j], m_softeningSquared);
            j++;
            bodyBodyInteraction<T>(acc, &m_pos[4*i], &m_pos[4*j], m_softeningSquared);
            j++;
        }

        m_force[indexForce  ] = acc[0];
        m_force[indexForce+1] = acc[1];
        m_force[indexForce+2] = acc[2];
    }
}

template <typename T>
struct _integrateNBodySystem__args {
  T deltaTime;
  T* m_pos;
  T* m_vel;
  T* m_force;

}

template <typename T>
void _integrateNBodySystem_launcher(omp_offloading_t * off, void *args) {
    struct _integrateNBodySystem__args * iargs = (struct _integrateNBodySystem__args*) args;
    T deltaTime = iargs->deltaTime;
    omp_offloading_info_t * off_info = off->off_info;
    omp_data_map_t * map_pos = omp_map_get_map(off, iargs->m_pos, -1); 
    omp_data_map_t * map_vel = omp_map_get_map(off, iargs->m_vel, -1); 
    omp_data_map_t * map_force = omp_map_get_map(off, iargs->m_force, -1); 

    T* pos_p = (T *)map_pos->map_dev_ptr;
    T* vel_p = (T *)map_vel->map_dev_ptr;
    T* force_p = (T *)map_force->map_dev_ptr;

    omp_device_type_t devtype = off_info->targets[off->devseqid]->type;
#if defined (DEVICE_NVGPU_SUPPORT)
    if (devtype == OMP_DEVICE_NVGPU) {
	int threads_per_team = omp_get_optimal_threads_per_team(off->dev);
	int teams_per_league = (n*m + threads_per_team - 1) / threads_per_team;
// Need to add GPU function here
	//OUT__2__10550__<<<teams_per_league, threads_per_team, 0,off->stream.systream.cudaStream>>>(n, m,u,uold);

    } else
    for (int i = 0; i < m_numBodies; ++i)
    {
        int index = 4*i;
        int indexForce = 3*i;


        T pos[3], vel[3], force[3];
        pos[0] = pos_p[index+0];
        pos[1] = pos_p[index+1];
        pos[2] = pos_p[index+2];
        T invMass = pos_p[index+3];

        vel[0] = vel_p[index+0];
        vel[1] = vel_p[index+1];
        vel[2] = vel_p[index+2];

        force[0] = force_p[indexForce+0];
        force[1] = force_p[indexForce+1];
        force[2] = force_p[indexForce+2];

        // acceleration = force / mass;
        // new velocity = old velocity + acceleration * deltaTime
        vel[0] += (force[0] * invMass) * deltaTime;
        vel[1] += (force[1] * invMass) * deltaTime;
        vel[2] += (force[2] * invMass) * deltaTime;

        vel[0] *= m_damping;
        vel[1] *= m_damping;
        vel[2] *= m_damping;

        // new position = old position + velocity * deltaTime
        pos[0] += vel[0] * deltaTime;
        pos[1] += vel[1] * deltaTime;
        pos[2] += vel[2] * deltaTime;

        pos_p[index+0] = pos[0];
        pos_p[index+1] = pos[1];
        pos_p[index+2] = pos[2];

        vel_p[index+0] = vel[0];
        vel_p[index+1] = vel[1];
        vel_p[index+2] = vel[2];
    }
}

template <typename T>
void BodySystemCPU<T>::_integrateNBodySystem(T deltaTime)
{
    _computeNBodyGravitation();
#ifdef OPENMP
    #pragma omp parallel for
#endif
	double ompacc_time = read_timer_ms(); //read_timer_ms();
	
    /* get number of target devices specified by the programmers */
    int __num_target_devices__ = omp_get_num_active_devices(); /*XXX: = runtime or compiler generated code */
    
	omp_device_t *__target_devices__[__num_target_devices__];
	/**TODO: compiler generated code or runtime call to init the __target_devices__ array */
	int __i__;
	for (__i__ = 0; __i__ < __num_target_devices__; __i__++) {
		__target_devices__[__i__] = &omp_devices[__i__]; /* currently this is simple a copy of the pointer */
	}
	/**TODO: compiler generated code or runtime call to init the topology */
	omp_grid_topology_t __top__;	
	int __top_ndims__ = 1;
	int __top_dims__[__top_ndims__];
	int __top_periodic__[__top_ndims__]; 
	int __id_map__[__num_target_devices__];
	omp_grid_topology_init_simple (&__top__, __target_devices__, __num_target_devices__, __top_ndims__, __top_dims__, __top_periodic__, __id_map__);

	int __num_mapped_array__ = 3; /* XXX: need compiler output */

	omp_data_map_info_t __data_map_infos__[__num_mapped_array__];
		
	omp_data_map_info_t * __info__ = &__data_map_infos__[0];
	long pos_dims[1]; pos_dims[0] = 4*m_numBodies;
	omp_data_map_t pos_maps[__num_target_devices__];
	omp_data_map_dist_t pos_dist[1];
	omp_data_map_init_info_straight_dist(__info__, &__top__, pos, 1, pos_dims, sizeof(REAL), pos_maps, OMP_DATA_MAP_TO, pos_dist, OMP_DATA_MAP_DIST_EVEN);

	omp_data_map_info_t * __info__ = &__data_map_infos__[1];
	long vel_dims[1]; vel_dims[0] = 3*m_numBodies;
	omp_data_map_t vel_maps[__num_target_devices__];
	omp_data_map_dist_t vel_dist[1];
	omp_data_map_init_info_straight_dist(__info__, &__top__, vel, 1, vel_dims, sizeof(REAL), vel_maps, OMP_DATA_MAP_TO, vel_dist, OMP_DATA_MAP_DIST_EVEN);

	omp_data_map_info_t * __info__ = &__data_map_infos__[2];
	long force_dims[1]; force_dims[0] = 3*m_numBodies;
	omp_data_map_t force_maps[__num_target_devices__];
	omp_data_map_dist_t force_dist[1];
	omp_data_map_init_info_straight_dist(__info__, &__top__, force, 1, force_dims, sizeof(REAL), force_maps, OMP_DATA_MAP_TO, force_dist, OMP_DATA_MAP_DIST_EVEN);

	struct _integrateNBodySystem__args args; 
        args.deltaTime = deltaTime; 
        args.m_pos = m_pos; 
        args.m_vel = m_vel; 
        args.m_force = m_force; 

	omp_offloading_info_t __offloading_info__;
	__offloading_info__.offloadings = (omp_offloading_t *) alloca(sizeof(omp_offloading_t) * __num_target_devices__);
	/* we use universal args and launcher because axpy can do it */
	omp_offloading_init_info(&__offloading_info__, &__top__, __target_devices__, OMP_OFFLOADING_DATA_CODE, __num_mapped_array__, __data_map_infos__, _integrateNBodySystem_launcher, &args);
	omp_offloading_start(__target_devices__, __num_target_devices__, &__offloading_info__);

	ompacc_time = read_timer_ms() - ompacc_time;
	double cpu_total = ompacc_time;


}
