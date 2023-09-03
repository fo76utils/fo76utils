
#ifndef NIFBLOCK_CPP_INCLUDED
#define NIFBLOCK_CPP_INCLUDED

const char * NIFFile::blockTypeStrings[169] =
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
  "BSGeometry",                                 //  20
  "BSInvMarker",                                //  21
  "BSLODTriShape",                              //  22
  "BSLagBoneController",                        //  23
  "BSLeafAnimNode",                             //  24
  "BSLightingShaderProperty",                   //  25
  "BSLightingShaderPropertyColorController",    //  26
  "BSLightingShaderPropertyFloatController",    //  27
  "BSMasterParticleSystem",                     //  28
  "BSMeshLODTriShape",                          //  29
  "BSMultiBound",                               //  30
  "BSMultiBoundAABB",                           //  31
  "BSMultiBoundNode",                           //  32
  "BSMultiBoundOBB",                            //  33
  "BSNiAlphaPropertyTestRefController",         //  34
  "BSOrderedNode",                              //  35
  "BSPSysHavokUpdateModifier",                  //  36
  "BSPSysInheritVelocityModifier",              //  37
  "BSPSysLODModifier",                          //  38
  "BSPSysMultiTargetEmitterCtlr",               //  39
  "BSPSysRecycleBoundModifier",                 //  40
  "BSPSysScaleModifier",                        //  41
  "BSPSysSimpleColorModifier",                  //  42
  "BSPSysStripUpdateModifier",                  //  43
  "BSPSysSubTexModifier",                       //  44
  "BSParentVelocityModifier",                   //  45
  "BSPositionData",                             //  46
  "BSProceduralLightningController",            //  47
  "BSShaderTextureSet",                         //  48
  "BSSkin::BoneData",                           //  49
  "BSSkin::Instance",                           //  50
  "BSSkyShaderProperty",                        //  51
  "BSStripPSysData",                            //  52
  "BSStripParticleSystem",                      //  53
  "BSSubIndexTriShape",                         //  54
  "BSTreeNode",                                 //  55
  "BSTriShape",                                 //  56
  "BSValueNode",                                //  57
  "BSWaterShaderProperty",                      //  58
  "BSWindModifier",                             //  59
  "BSXFlags",                                   //  60
  "NiAlphaProperty",                            //  61
  "NiBSBoneLODController",                      //  62
  "NiBillboardNode",                            //  63
  "NiBinaryExtraData",                          //  64
  "NiBlendBoolInterpolator",                    //  65
  "NiBlendFloatInterpolator",                   //  66
  "NiBlendPoint3Interpolator",                  //  67
  "NiBoolData",                                 //  68
  "NiBoolInterpolator",                         //  69
  "NiBoolTimelineInterpolator",                 //  70
  "NiBooleanExtraData",                         //  71
  "NiCamera",                                   //  72
  "NiControllerManager",                        //  73
  "NiControllerSequence",                       //  74
  "NiDefaultAVObjectPalette",                   //  75
  "NiExtraData",                                //  76
  "NiFloatData",                                //  77
  "NiFloatExtraData",                           //  78
  "NiFloatExtraDataController",                 //  79
  "NiFloatInterpolator",                        //  80
  "NiIntegerExtraData",                         //  81
  "NiLightColorController",                     //  82
  "NiLightDimmerController",                    //  83
  "NiLightRadiusController",                    //  84
  "NiLookAtInterpolator",                       //  85
  "NiMeshPSysData",                             //  86
  "NiMeshParticleSystem",                       //  87
  "NiMultiTargetTransformController",           //  88
  "NiNode",                                     //  89
  "NiPSysAgeDeathModifier",                     //  90
  "NiPSysBombModifier",                         //  91
  "NiPSysBoundUpdateModifier",                  //  92
  "NiPSysBoxEmitter",                           //  93
  "NiPSysColliderManager",                      //  94
  "NiPSysCylinderEmitter",                      //  95
  "NiPSysData",                                 //  96
  "NiPSysDragModifier",                         //  97
  "NiPSysEmitterCtlr",                          //  98
  "NiPSysEmitterDeclinationCtlr",               //  99
  "NiPSysEmitterDeclinationVarCtlr",            // 100
  "NiPSysEmitterInitialRadiusCtlr",             // 101
  "NiPSysEmitterLifeSpanCtlr",                  // 102
  "NiPSysEmitterPlanarAngleCtlr",               // 103
  "NiPSysEmitterPlanarAngleVarCtlr",            // 104
  "NiPSysEmitterSpeedCtlr",                     // 105
  "NiPSysGravityModifier",                      // 106
  "NiPSysGravityStrengthCtlr",                  // 107
  "NiPSysInitialRotSpeedCtlr",                  // 108
  "NiPSysInitialRotSpeedVarCtlr",               // 109
  "NiPSysMeshEmitter",                          // 110
  "NiPSysModifierActiveCtlr",                   // 111
  "NiPSysPlanarCollider",                       // 112
  "NiPSysPositionModifier",                     // 113
  "NiPSysResetOnLoopCtlr",                      // 114
  "NiPSysRotationModifier",                     // 115
  "NiPSysSpawnModifier",                        // 116
  "NiPSysSphereEmitter",                        // 117
  "NiPSysSphericalCollider",                    // 118
  "NiPSysUpdateCtlr",                           // 119
  "NiParticleSystem",                           // 120
  "NiPathInterpolator",                         // 121
  "NiPoint3Interpolator",                       // 122
  "NiPointLight",                               // 123
  "NiPosData",                                  // 124
  "NiSkinData",                                 // 125
  "NiSkinInstance",                             // 126
  "NiSkinPartition",                            // 127
  "NiSpotLight",                                // 128
  "NiStringExtraData",                          // 129
  "NiStringsExtraData",                         // 130
  "NiSwitchNode",                               // 131
  "NiTextKeyExtraData",                         // 132
  "NiTransformController",                      // 133
  "NiTransformData",                            // 134
  "NiTransformInterpolator",                    // 135
  "NiTriShape",                                 // 136
  "NiTriShapeData",                             // 137
  "NiTriStripsData",                            // 138
  "NiVisController",                            // 139
  "bhkBallAndSocketConstraint",                 // 140
  "bhkBallSocketConstraintChain",               // 141
  "bhkBlendCollisionObject",                    // 142
  "bhkBoxShape",                                // 143
  "bhkBreakableConstraint",                     // 144
  "bhkCapsuleShape",                            // 145
  "bhkCollisionObject",                         // 146
  "bhkCompressedMeshShape",                     // 147
  "bhkCompressedMeshShapeData",                 // 148
  "bhkConvexTransformShape",                    // 149
  "bhkConvexVerticesShape",                     // 150
  "bhkCylinderShape",                           // 151
  "bhkHingeConstraint",                         // 152
  "bhkLimitedHingeConstraint",                  // 153
  "bhkListShape",                               // 154
  "bhkMoppBvTreeShape",                         // 155
  "bhkNPCollisionObject",                       // 156
  "bhkNiTriStripsShape",                        // 157
  "bhkPhysicsSystem",                           // 158
  "bhkPlaneShape",                              // 159
  "bhkRagdollConstraint",                       // 160
  "bhkRagdollSystem",                           // 161
  "bhkRigidBody",                               // 162
  "bhkRigidBodyT",                              // 163
  "bhkSPCollisionObject",                       // 164
  "bhkSimpleShapePhantom",                      // 165
  "bhkSphereShape",                             // 166
  "bhkStiffSpringConstraint",                   // 167
  "bhkTransformShape"                           // 168
};

const unsigned char NIFFile::blockTypeBaseTable[169] =
{
    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,     //   0
   10,  11,  12,  25,  14,  15,  16,  89,  18,  19,     //  10
   20,  21,  22,  23,  89,  25,  26,  27,  28,  56,     //  20
   30,  31,  89,  33,  34,  89,  36,  37,  38,  39,     //  30
   40,  41,  42,  43,  44,  45,  46,  47,  48,  49,     //  40
   50,  51,  52,  53,  56,  89,  56,  57,  58,  59,     //  50
   60,  61,  62,  63,  64,  65,  66,  67,  68,  69,     //  60
   70,  71,  72,  73,  74,  75,  76,  77,  78,  79,     //  70
   80,  81,  82,  83,  84,  85,  86,  87,  88,  89,     //  80
   90,  91,  92,  93,  94,  95,  96,  97,  98,  99,     //  90
  100, 101, 102, 103, 104, 105, 106, 107, 108, 109,     // 100
  110, 111, 112, 113, 114, 115, 116, 117, 118, 119,     // 110
  120, 121, 122, 123, 124, 125, 126, 127, 128, 129,     // 120
  130,  89, 132, 133, 134, 135, 136, 137, 138, 139,     // 130
  140, 141, 142, 143, 144, 145, 146, 147, 148, 149,     // 140
  150, 151, 152, 153, 154, 155, 156, 157, 158, 159,     // 150
  160, 161, 162, 163, 164, 165, 166, 167, 168           // 160
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

