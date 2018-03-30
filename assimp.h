/*!\file assimp.h
 * \author Farès BELHADJ, hatim tachi
 * \date February 14, 2017
 */

#ifndef _ASSIMP_H

#define _ASSIMP_H

#ifdef __cplusplus
extern "C" {
#endif

  extern void assimpInit(const char * filename, int idObject);
  extern void assimpDrawScene(int idObject);
  extern void assimpQuit(void);
  
#ifdef __cplusplus
}
#endif

#endif

