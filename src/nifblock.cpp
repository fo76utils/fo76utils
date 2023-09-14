
#ifndef NIFBLOCK_CPP_INCLUDED
#define NIFBLOCK_CPP_INCLUDED

const char * NIFFile::blockTypeStrings[174] =
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
  "BSFaceGenNiNode",                            //  17
  "BSFadeNode",                                 //  18
  "BSFrustumFOVController",                     //  19
  "BSFurnitureMarkerNode",                      //  20
  "BSGeometry",                                 //  21
  "BSInvMarker",                                //  22
  "BSLODTriShape",                              //  23
  "BSLagBoneController",                        //  24
  "BSLeafAnimNode",                             //  25
  "BSLightingShaderProperty",                   //  26
  "BSLightingShaderPropertyColorController",    //  27
  "BSLightingShaderPropertyFloatController",    //  28
  "BSMasterParticleSystem",                     //  29
  "BSMeshLODTriShape",                          //  30
  "BSMultiBound",                               //  31
  "BSMultiBoundAABB",                           //  32
  "BSMultiBoundNode",                           //  33
  "BSMultiBoundOBB",                            //  34
  "BSNiAlphaPropertyTestRefController",         //  35
  "BSOrderedNode",                              //  36
  "BSPSysHavokUpdateModifier",                  //  37
  "BSPSysInheritVelocityModifier",              //  38
  "BSPSysLODModifier",                          //  39
  "BSPSysMultiTargetEmitterCtlr",               //  40
  "BSPSysRecycleBoundModifier",                 //  41
  "BSPSysScaleModifier",                        //  42
  "BSPSysSimpleColorModifier",                  //  43
  "BSPSysStripUpdateModifier",                  //  44
  "BSPSysSubTexModifier",                       //  45
  "BSParentVelocityModifier",                   //  46
  "BSPositionData",                             //  47
  "BSProceduralLightningController",            //  48
  "BSShaderTextureSet",                         //  49
  "BSSkin::BoneData",                           //  50
  "BSSkin::Instance",                           //  51
  "BSSkyShaderProperty",                        //  52
  "BSStripPSysData",                            //  53
  "BSStripParticleSystem",                      //  54
  "BSSubIndexTriShape",                         //  55
  "BSTreeNode",                                 //  56
  "BSTriShape",                                 //  57
  "BSValueNode",                                //  58
  "BSWaterShaderProperty",                      //  59
  "BSWeakReferenceNode",                        //  60
  "BSWindModifier",                             //  61
  "BSXFlags",                                   //  62
  "BoneTranslations",                           //  63
  "NiAlphaProperty",                            //  64
  "NiBSBoneLODController",                      //  65
  "NiBillboardNode",                            //  66
  "NiBinaryExtraData",                          //  67
  "NiBlendBoolInterpolator",                    //  68
  "NiBlendFloatInterpolator",                   //  69
  "NiBlendPoint3Interpolator",                  //  70
  "NiBoolData",                                 //  71
  "NiBoolInterpolator",                         //  72
  "NiBoolTimelineInterpolator",                 //  73
  "NiBooleanExtraData",                         //  74
  "NiCamera",                                   //  75
  "NiControllerManager",                        //  76
  "NiControllerSequence",                       //  77
  "NiDefaultAVObjectPalette",                   //  78
  "NiExtraData",                                //  79
  "NiFloatData",                                //  80
  "NiFloatExtraData",                           //  81
  "NiFloatExtraDataController",                 //  82
  "NiFloatInterpolator",                        //  83
  "NiIntegerExtraData",                         //  84
  "NiIntegersExtraData",                        //  85
  "NiLightColorController",                     //  86
  "NiLightDimmerController",                    //  87
  "NiLightRadiusController",                    //  88
  "NiLookAtInterpolator",                       //  89
  "NiMeshPSysData",                             //  90
  "NiMeshParticleSystem",                       //  91
  "NiMultiTargetTransformController",           //  92
  "NiNode",                                     //  93
  "NiPSysAgeDeathModifier",                     //  94
  "NiPSysBombModifier",                         //  95
  "NiPSysBoundUpdateModifier",                  //  96
  "NiPSysBoxEmitter",                           //  97
  "NiPSysColliderManager",                      //  98
  "NiPSysCylinderEmitter",                      //  99
  "NiPSysData",                                 // 100
  "NiPSysDragModifier",                         // 101
  "NiPSysEmitterCtlr",                          // 102
  "NiPSysEmitterDeclinationCtlr",               // 103
  "NiPSysEmitterDeclinationVarCtlr",            // 104
  "NiPSysEmitterInitialRadiusCtlr",             // 105
  "NiPSysEmitterLifeSpanCtlr",                  // 106
  "NiPSysEmitterPlanarAngleCtlr",               // 107
  "NiPSysEmitterPlanarAngleVarCtlr",            // 108
  "NiPSysEmitterSpeedCtlr",                     // 109
  "NiPSysGravityModifier",                      // 110
  "NiPSysGravityStrengthCtlr",                  // 111
  "NiPSysInitialRotSpeedCtlr",                  // 112
  "NiPSysInitialRotSpeedVarCtlr",               // 113
  "NiPSysMeshEmitter",                          // 114
  "NiPSysModifierActiveCtlr",                   // 115
  "NiPSysPlanarCollider",                       // 116
  "NiPSysPositionModifier",                     // 117
  "NiPSysResetOnLoopCtlr",                      // 118
  "NiPSysRotationModifier",                     // 119
  "NiPSysSpawnModifier",                        // 120
  "NiPSysSphereEmitter",                        // 121
  "NiPSysSphericalCollider",                    // 122
  "NiPSysUpdateCtlr",                           // 123
  "NiParticleSystem",                           // 124
  "NiPathInterpolator",                         // 125
  "NiPoint3Interpolator",                       // 126
  "NiPointLight",                               // 127
  "NiPosData",                                  // 128
  "NiSkinData",                                 // 129
  "NiSkinInstance",                             // 130
  "NiSkinPartition",                            // 131
  "NiSpotLight",                                // 132
  "NiStringExtraData",                          // 133
  "NiStringsExtraData",                         // 134
  "NiSwitchNode",                               // 135
  "NiTextKeyExtraData",                         // 136
  "NiTransformController",                      // 137
  "NiTransformData",                            // 138
  "NiTransformInterpolator",                    // 139
  "NiTriShape",                                 // 140
  "NiTriShapeData",                             // 141
  "NiTriStripsData",                            // 142
  "NiVisController",                            // 143
  "SkinAttach",                                 // 144
  "bhkBallAndSocketConstraint",                 // 145
  "bhkBallSocketConstraintChain",               // 146
  "bhkBlendCollisionObject",                    // 147
  "bhkBoxShape",                                // 148
  "bhkBreakableConstraint",                     // 149
  "bhkCapsuleShape",                            // 150
  "bhkCollisionObject",                         // 151
  "bhkCompressedMeshShape",                     // 152
  "bhkCompressedMeshShapeData",                 // 153
  "bhkConvexTransformShape",                    // 154
  "bhkConvexVerticesShape",                     // 155
  "bhkCylinderShape",                           // 156
  "bhkHingeConstraint",                         // 157
  "bhkLimitedHingeConstraint",                  // 158
  "bhkListShape",                               // 159
  "bhkMoppBvTreeShape",                         // 160
  "bhkNPCollisionObject",                       // 161
  "bhkNiTriStripsShape",                        // 162
  "bhkPhysicsSystem",                           // 163
  "bhkPlaneShape",                              // 164
  "bhkRagdollConstraint",                       // 165
  "bhkRagdollSystem",                           // 166
  "bhkRigidBody",                               // 167
  "bhkRigidBodyT",                              // 168
  "bhkSPCollisionObject",                       // 169
  "bhkSimpleShapePhantom",                      // 170
  "bhkSphereShape",                             // 171
  "bhkStiffSpringConstraint",                   // 172
  "bhkTransformShape"                           // 173
};

const unsigned char NIFFile::blockTypeBaseTable[174] =
{
    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,     //   0
   10,  11,  12,  26,  14,  15,  16,  93,  93,  19,     //  10
   20,  57,  22,  23,  24,  93,  26,  27,  28,  29,     //  20
   57,  31,  32,  93,  34,  35,  93,  37,  38,  39,     //  30
   40,  41,  42,  43,  44,  45,  46,  47,  48,  49,     //  40
   50,  51,  52,  53,  54,  57,  93,  57,  58,  59,     //  50
   60,  61,  62,  63,  64,  65,  66,  67,  68,  69,     //  60
   70,  71,  72,  73,  74,  75,  76,  77,  78,  79,     //  70
   80,  81,  82,  83,  84,  85,  86,  87,  88,  89,     //  80
   90,  91,  92,  93,  94,  95,  96,  97,  98,  99,     //  90
  100, 101, 102, 103, 104, 105, 106, 107, 108, 109,     // 100
  110, 111, 112, 113, 114, 115, 116, 117, 118, 119,     // 110
  120, 121, 122, 123, 124, 125, 126, 127, 128, 129,     // 120
  130, 131, 132, 133, 134,  93, 136, 137, 138, 139,     // 130
  140, 141, 142, 143, 144, 145, 146, 147, 148, 149,     // 140
  150, 151, 152, 153, 154, 155, 156, 157, 158, 159,     // 150
  160, 161, 162, 163, 164, 165, 166, 167, 168, 169,     // 160
  170, 171, 172, 173                                    // 170
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

