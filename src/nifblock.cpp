
#ifndef NIFBLOCK_CPP_INCLUDED
#define NIFBLOCK_CPP_INCLUDED

const char * NIFFile::blockTypeStrings[168] =
{
  "Unknown",                                    //   0
  "BSBehaviorGraphExtraData",                   //   1
  "BSBlastNode",                                //   2
  "BSBoneLODExtraData",                         //   3
  "BSBound",                                    //   4
  "BSClothExtraData",                           //   5
  "BSCollisionQueryProxyExtraData",             //   6
  "BSConnectPoint::Children",                   //   7
  "BSConnectPoint::Parents",                    //   8
  "BSDamageStage",                              //   9
  "BSDecalPlacementVectorExtraData",            //  10
  "BSDismemberSkinInstance",                    //  11
  "BSDynamicTriShape",                          //  12
  "BSEffectShaderProperty",                     //  13
  "BSEffectShaderPropertyColorController",      //  14
  "BSEffectShaderPropertyFloatController",      //  15
  "BSEyeCenterExtraData",                       //  16
  "BSFadeNode",                                 //  17
  "BSFrustumFOVController",                     //  18
  "BSFurnitureMarkerNode",                      //  19
  "BSInvMarker",                                //  20
  "BSLODTriShape",                              //  21
  "BSLagBoneController",                        //  22
  "BSLeafAnimNode",                             //  23
  "BSLightingShaderProperty",                   //  24
  "BSLightingShaderPropertyColorController",    //  25
  "BSLightingShaderPropertyFloatController",    //  26
  "BSMasterParticleSystem",                     //  27
  "BSMeshLODTriShape",                          //  28
  "BSMultiBound",                               //  29
  "BSMultiBoundAABB",                           //  30
  "BSMultiBoundNode",                           //  31
  "BSMultiBoundOBB",                            //  32
  "BSNiAlphaPropertyTestRefController",         //  33
  "BSOrderedNode",                              //  34
  "BSPSysHavokUpdateModifier",                  //  35
  "BSPSysInheritVelocityModifier",              //  36
  "BSPSysLODModifier",                          //  37
  "BSPSysMultiTargetEmitterCtlr",               //  38
  "BSPSysRecycleBoundModifier",                 //  39
  "BSPSysScaleModifier",                        //  40
  "BSPSysSimpleColorModifier",                  //  41
  "BSPSysStripUpdateModifier",                  //  42
  "BSPSysSubTexModifier",                       //  43
  "BSParentVelocityModifier",                   //  44
  "BSPositionData",                             //  45
  "BSProceduralLightningController",            //  46
  "BSShaderTextureSet",                         //  47
  "BSSkin::BoneData",                           //  48
  "BSSkin::Instance",                           //  49
  "BSSkyShaderProperty",                        //  50
  "BSStripPSysData",                            //  51
  "BSStripParticleSystem",                      //  52
  "BSSubIndexTriShape",                         //  53
  "BSTreeNode",                                 //  54
  "BSTriShape",                                 //  55
  "BSValueNode",                                //  56
  "BSWaterShaderProperty",                      //  57
  "BSWindModifier",                             //  58
  "BSXFlags",                                   //  59
  "NiAlphaProperty",                            //  60
  "NiBSBoneLODController",                      //  61
  "NiBillboardNode",                            //  62
  "NiBinaryExtraData",                          //  63
  "NiBlendBoolInterpolator",                    //  64
  "NiBlendFloatInterpolator",                   //  65
  "NiBlendPoint3Interpolator",                  //  66
  "NiBoolData",                                 //  67
  "NiBoolInterpolator",                         //  68
  "NiBoolTimelineInterpolator",                 //  69
  "NiBooleanExtraData",                         //  70
  "NiCamera",                                   //  71
  "NiControllerManager",                        //  72
  "NiControllerSequence",                       //  73
  "NiDefaultAVObjectPalette",                   //  74
  "NiExtraData",                                //  75
  "NiFloatData",                                //  76
  "NiFloatExtraData",                           //  77
  "NiFloatExtraDataController",                 //  78
  "NiFloatInterpolator",                        //  79
  "NiIntegerExtraData",                         //  80
  "NiLightColorController",                     //  81
  "NiLightDimmerController",                    //  82
  "NiLightRadiusController",                    //  83
  "NiLookAtInterpolator",                       //  84
  "NiMeshPSysData",                             //  85
  "NiMeshParticleSystem",                       //  86
  "NiMultiTargetTransformController",           //  87
  "NiNode",                                     //  88
  "NiPSysAgeDeathModifier",                     //  89
  "NiPSysBombModifier",                         //  90
  "NiPSysBoundUpdateModifier",                  //  91
  "NiPSysBoxEmitter",                           //  92
  "NiPSysColliderManager",                      //  93
  "NiPSysCylinderEmitter",                      //  94
  "NiPSysData",                                 //  95
  "NiPSysDragModifier",                         //  96
  "NiPSysEmitterCtlr",                          //  97
  "NiPSysEmitterDeclinationCtlr",               //  98
  "NiPSysEmitterDeclinationVarCtlr",            //  99
  "NiPSysEmitterInitialRadiusCtlr",             // 100
  "NiPSysEmitterLifeSpanCtlr",                  // 101
  "NiPSysEmitterPlanarAngleCtlr",               // 102
  "NiPSysEmitterPlanarAngleVarCtlr",            // 103
  "NiPSysEmitterSpeedCtlr",                     // 104
  "NiPSysGravityModifier",                      // 105
  "NiPSysGravityStrengthCtlr",                  // 106
  "NiPSysInitialRotSpeedCtlr",                  // 107
  "NiPSysInitialRotSpeedVarCtlr",               // 108
  "NiPSysMeshEmitter",                          // 109
  "NiPSysModifierActiveCtlr",                   // 110
  "NiPSysPlanarCollider",                       // 111
  "NiPSysPositionModifier",                     // 112
  "NiPSysResetOnLoopCtlr",                      // 113
  "NiPSysRotationModifier",                     // 114
  "NiPSysSpawnModifier",                        // 115
  "NiPSysSphereEmitter",                        // 116
  "NiPSysSphericalCollider",                    // 117
  "NiPSysUpdateCtlr",                           // 118
  "NiParticleSystem",                           // 119
  "NiPathInterpolator",                         // 120
  "NiPoint3Interpolator",                       // 121
  "NiPointLight",                               // 122
  "NiPosData",                                  // 123
  "NiSkinData",                                 // 124
  "NiSkinInstance",                             // 125
  "NiSkinPartition",                            // 126
  "NiSpotLight",                                // 127
  "NiStringExtraData",                          // 128
  "NiStringsExtraData",                         // 129
  "NiSwitchNode",                               // 130
  "NiTextKeyExtraData",                         // 131
  "NiTransformController",                      // 132
  "NiTransformData",                            // 133
  "NiTransformInterpolator",                    // 134
  "NiTriShape",                                 // 135
  "NiTriShapeData",                             // 136
  "NiTriStripsData",                            // 137
  "NiVisController",                            // 138
  "bhkBallAndSocketConstraint",                 // 139
  "bhkBallSocketConstraintChain",               // 140
  "bhkBlendCollisionObject",                    // 141
  "bhkBoxShape",                                // 142
  "bhkBreakableConstraint",                     // 143
  "bhkCapsuleShape",                            // 144
  "bhkCollisionObject",                         // 145
  "bhkCompressedMeshShape",                     // 146
  "bhkCompressedMeshShapeData",                 // 147
  "bhkConvexTransformShape",                    // 148
  "bhkConvexVerticesShape",                     // 149
  "bhkCylinderShape",                           // 150
  "bhkHingeConstraint",                         // 151
  "bhkLimitedHingeConstraint",                  // 152
  "bhkListShape",                               // 153
  "bhkMoppBvTreeShape",                         // 154
  "bhkNPCollisionObject",                       // 155
  "bhkNiTriStripsShape",                        // 156
  "bhkPhysicsSystem",                           // 157
  "bhkPlaneShape",                              // 158
  "bhkRagdollConstraint",                       // 159
  "bhkRagdollSystem",                           // 160
  "bhkRigidBody",                               // 161
  "bhkRigidBodyT",                              // 162
  "bhkSPCollisionObject",                       // 163
  "bhkSimpleShapePhantom",                      // 164
  "bhkSphereShape",                             // 165
  "bhkStiffSpringConstraint",                   // 166
  "bhkTransformShape"                           // 167
};

const unsigned char NIFFile::blockTypeBaseTable[168] =
{
    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,     //   0
   10,  11,  12,  13,  14,  15,  16,  88,  18,  19,     //  10
   20,  21,  22,  88,  24,  25,  26,  27,  55,  29,     //  20
   30,  88,  32,  33,  34,  35,  36,  37,  38,  39,     //  30
   40,  41,  42,  43,  44,  45,  46,  47,  48,  49,     //  40
   50,  51,  52,  55,  88,  55,  56,  57,  58,  59,     //  50
   60,  61,  62,  63,  64,  65,  66,  67,  68,  69,     //  60
   70,  71,  72,  73,  74,  75,  76,  77,  78,  79,     //  70
   80,  81,  82,  83,  84,  85,  86,  87,  88,  89,     //  80
   90,  91,  92,  93,  94,  95,  96,  97,  98,  99,     //  90
  100, 101, 102, 103, 104, 105, 106, 107, 108, 109,     // 100
  110, 111, 112, 113, 114, 115, 116, 117, 118, 119,     // 110
  120, 121, 122, 123, 124, 125, 126, 127, 128, 129,     // 120
   88, 131, 132, 133, 134, 135, 136, 137, 138, 139,     // 130
  140, 141, 142, 143, 144, 145, 146, 147, 148, 149,     // 140
  150, 151, 152, 153, 154, 155, 156, 157, 158, 159,     // 150
  160, 161, 162, 163, 164, 165, 166, 167                // 160
};

int NIFFile::stringToBlockType(const char *s)
{
  if (!s)
    return BlkTypeUnknown;
  int     n0 = 1;
  int     n2 = int(sizeof(blockTypeStrings) / sizeof(char *));
  while (n2 >= (n0 + 2))
  {
    int     n1 = (n0 + n2) >> 1;
    int     d = std::strcmp(s, blockTypeStrings[n1]);
    if (!d)
      return n1;
    if (d < 0)
      n2 = n1;
    else
      n0 = n1 + 1;
  }
  if (n0 >= n2 || std::strcmp(s, blockTypeStrings[n0]) != 0)
    return BlkTypeUnknown;
  return n0;
}

#endif

