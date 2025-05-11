#ifndef OPC_FREERTOS_STATUS_H
#define OPC_FREERTOS_STATUS_H

#include "open62541.h"
#include <FreeRTOS.h>
#include <stdio.h>

typedef struct
{
    char name[40];    // Имя задачи
    uint32_t runtime; // Время выполнения (в % или тиках)
} TaskRuntimeInfo;

/**
 * ----------------------------------------------------------------------------------------------------
 * Declarations
 * ----------------------------------------------------------------------------------------------------
 */
static void addGetHeapStatsVariable(UA_Server *server);
static void addValueCallbackToGetHeapStatsVariable(UA_Server *server);
static void beforeWriteGetHeapStats(UA_Server *server,
                                    const UA_NodeId *sessionId, void *sessionContext,
                                    const UA_NodeId *nodeId, void *nodeContext,
                                    const UA_NumericRange *range, const UA_DataValue *data);
static void afterWriteGetHeapStats(UA_Server *server,
                                   const UA_NodeId *sessionId, void *sessionContext,
                                   const UA_NodeId *nodeid, void *nodeContext,
                                   const UA_NumericRange *range, const UA_DataValue *data);
static void updateGetHeapStatsNode(UA_Server *server);

static void addTaskStatsFolder(UA_Server *server);
static void addTaskStatsVariable(UA_Server *server, TaskHandle_t xTask);
static void updateGetTaskStatsNode(UA_Server *server, const char *pcName);
static void addValueCallbackToGetTaskStatsVariable(UA_Server *server, const char *pcName);
static void beforeWriteGetTaskStats(UA_Server *server,
                        const UA_NodeId *sessionId, void *sessionContext,
                        const UA_NodeId *nodeid, void *nodeContext,
                        const UA_NumericRange *range, const UA_DataValue *data);
static void afterWriteGetTaskStats(UA_Server *server,
                       const UA_NodeId *sessionId, void *sessionContext,
                       const UA_NodeId *nodeId, void *nodeContext,
                       const UA_NumericRange *range, const UA_DataValue *data);


/**
 * ----------------------------------------------------------------------------------------------------
 * vPortGetHeapStats
 * ----------------------------------------------------------------------------------------------------
 */
static void addGetHeapStatsVariable(UA_Server *server)
{
    UA_ObjectTypeAttributes objTypeAttr = UA_ObjectTypeAttributes_default;
    UA_VariableAttributes attr = UA_VariableAttributes_default;

    objTypeAttr.displayName = UA_LOCALIZEDTEXT("en-US", "HeapStatsType");

    UA_NodeId heapStatsTypeId = UA_NODEID_STRING(1, "HeapStatsType");
    UA_Server_addObjectTypeNode(
        server,
        heapStatsTypeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "HeapStatsType"),
        objTypeAttr,
        NULL,
        NULL);

    UA_ObjectAttributes objAttr = UA_ObjectAttributes_default;
    objAttr.displayName = UA_LOCALIZEDTEXT("en-US", "HeapStats");

    UA_NodeId heapStatsObjId = UA_NODEID_STRING(1, "HeapStats");
    UA_Server_addObjectNode(
        server,
        heapStatsObjId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "HeapStats"),
        heapStatsTypeId,
        objAttr,
        NULL,
        NULL);

    // Добавляем переменные как компоненты объекта
    UA_Server_addVariableNode(
        server,
        UA_NODEID_STRING(1, "HeapStats.xAvailableHeapSpaceInBytes"),
        heapStatsObjId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "xAvailableHeapSpaceInBytes"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr,
        NULL,
        NULL);

    UA_Server_addVariableNode(
        server,
        UA_NODEID_STRING(1, "HeapStats.xSizeOfLargestFreeBlockInBytes"),
        heapStatsObjId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "xSizeOfLargestFreeBlockInBytes"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr,
        NULL,
        NULL);

    UA_Server_addVariableNode(
        server,
        UA_NODEID_STRING(1, "HeapStats.xNumberOfFreeBlocks"),
        heapStatsObjId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "xNumberOfFreeBlocks"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr,
        NULL,
        NULL);

    UA_Server_addVariableNode(
        server,
        UA_NODEID_STRING(1, "HeapStats.xMinimumEverFreeBytesRemaining"),
        heapStatsObjId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "xMinimumEverFreeBytesRemaining"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr,
        NULL,
        NULL);

    UA_Server_addVariableNode(
        server,
        UA_NODEID_STRING(1, "HeapStats.xNumberOfSuccessfulAllocations"),
        heapStatsObjId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "xNumberOfSuccessfulAllocations"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr,
        NULL,
        NULL);

    UA_Server_addVariableNode(
        server,
        UA_NODEID_STRING(1, "HeapStats.xNumberOfSuccessfulFrees"),
        heapStatsObjId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "xNumberOfSuccessfulFrees"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr,
        NULL,
        NULL);

    updateGetHeapStatsNode(server);

    addValueCallbackToGetHeapStatsVariable(server);
}

static void updateGetHeapStatsNode(UA_Server *server)
{
    HeapStats_t pxHeapStats;
    vPortGetHeapStats(&pxHeapStats);

    // AvailableHeapSpace
    UA_Variant heapAvailablevalue;
    UA_UInt32 heapAvailable = (UA_UInt32)pxHeapStats.xAvailableHeapSpaceInBytes;
    UA_Variant_setScalar(&heapAvailablevalue, &heapAvailable, &UA_TYPES[UA_TYPES_UINT32]);
    UA_NodeId nodeAvailable = UA_NODEID_STRING(1, "HeapStats.xAvailableHeapSpaceInBytes");
    UA_Server_writeValue(server, nodeAvailable, heapAvailablevalue);

    // SizeOfLargestFreeBlockInBytes
    UA_Variant largestBlockvalue;
    UA_UInt32 largestBlock = (UA_UInt32)pxHeapStats.xSizeOfLargestFreeBlockInBytes;
    UA_Variant_setScalar(&largestBlockvalue, &largestBlock, &UA_TYPES[UA_TYPES_UINT32]);
    UA_NodeId nodeLargestBlock = UA_NODEID_STRING(1, "HeapStats.xSizeOfLargestFreeBlockInBytes");
    UA_Server_writeValue(server, nodeLargestBlock, largestBlockvalue);

    // SizeOfSmallestFreeBlockInBytes
    UA_Variant SmallestBlockvalue;
    UA_UInt32 SmallestFreeBlock = (UA_UInt32)pxHeapStats.xSizeOfSmallestFreeBlockInBytes;
    UA_Variant_setScalar(&SmallestBlockvalue, &SmallestFreeBlock, &UA_TYPES[UA_TYPES_UINT32]);
    UA_NodeId nodeSmallestFreeBlock = UA_NODEID_STRING(1, "HeapStats.xSizeOfSmallestFreeBlockInBytes");
    UA_Server_writeValue(server, nodeSmallestFreeBlock, SmallestBlockvalue);

    // NumberOfFreeBlocks
    UA_Variant freeBlocksvalue;
    UA_UInt32 freeBlocks = (UA_UInt32)pxHeapStats.xNumberOfFreeBlocks;
    UA_Variant_setScalar(&freeBlocksvalue, &freeBlocks, &UA_TYPES[UA_TYPES_UINT32]);
    UA_NodeId nodeFreeBlocks = UA_NODEID_STRING(1, "HeapStats.xNumberOfFreeBlocks");
    UA_Server_writeValue(server, nodeFreeBlocks, freeBlocksvalue);

    // MinimumEverFreeBytesRemaining
    UA_Variant EverFreeBytesvalue;
    UA_UInt32 EverFreeBytes = (UA_UInt32)pxHeapStats.xMinimumEverFreeBytesRemaining;
    UA_Variant_setScalar(&EverFreeBytesvalue, &EverFreeBytes, &UA_TYPES[UA_TYPES_UINT32]);
    UA_NodeId nodeEverFreeBytes = UA_NODEID_STRING(1, "HeapStats.xMinimumEverFreeBytesRemaining");
    UA_Server_writeValue(server, nodeEverFreeBytes, EverFreeBytesvalue);

    // NumberOfSuccessfulAllocations
    UA_Variant SuccessAllocationvalue;
    UA_UInt32 SuccessAllocation = (UA_UInt32)pxHeapStats.xNumberOfSuccessfulAllocations;
    UA_Variant_setScalar(&SuccessAllocationvalue, &SuccessAllocation, &UA_TYPES[UA_TYPES_UINT32]);
    UA_NodeId nodeSuccessAllocation = UA_NODEID_STRING(1, "HeapStats.xNumberOfSuccessfulAllocations");
    UA_Server_writeValue(server, nodeSuccessAllocation, SuccessAllocationvalue);

    // NumberOfSuccessfulFrees
    UA_Variant SuccessfulFreesvalue;
    UA_UInt32 SuccessfulFrees = (UA_UInt32)pxHeapStats.xNumberOfSuccessfulFrees;
    UA_Variant_setScalar(&SuccessfulFreesvalue, &SuccessfulFrees, &UA_TYPES[UA_TYPES_UINT32]);
    UA_NodeId nodeSuccessfulFrees = UA_NODEID_STRING(1, "HeapStats.xNumberOfSuccessfulFrees");
    UA_Server_writeValue(server, nodeSuccessfulFrees, SuccessfulFreesvalue);
}

static void
beforeWriteGetHeapStats(UA_Server *server,
                        const UA_NodeId *sessionId, void *sessionContext,
                        const UA_NodeId *nodeid, void *nodeContext,
                        const UA_NumericRange *range, const UA_DataValue *data)
{
    updateGetHeapStatsNode(server);
}

static void
afterWriteGetHeapStats(UA_Server *server,
                       const UA_NodeId *sessionId, void *sessionContext,
                       const UA_NodeId *nodeId, void *nodeContext,
                       const UA_NumericRange *range, const UA_DataValue *data)
{
}

static void
addValueCallbackToGetHeapStatsVariable(UA_Server *server)
{
    UA_NodeId currentNodeId = UA_NODEID_STRING(1, "HeapStats.xAvailableHeapSpaceInBytes");
    UA_ValueCallback callback;
    callback.onRead = beforeWriteGetHeapStats;
    callback.onWrite = afterWriteGetHeapStats;
    UA_Server_setVariableNode_valueCallback(server, currentNodeId, callback);
}

/**
 * ----------------------------------------------------------------------------------------------------
 * vTaskGetInfo
 * ----------------------------------------------------------------------------------------------------
 */
static void addTaskStatsFolder(UA_Server *server)
{
    UA_NodeId taskStatsFolderId = UA_NODEID_STRING(1, "TaskStats");
    UA_ObjectAttributes folderAttr = UA_ObjectAttributes_default;
    folderAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Task Statistics");

    UA_Server_addObjectNode(
        server,
        taskStatsFolderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "TaskStats"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),
        folderAttr,
        NULL,
        NULL);
}

static void addTaskStatsVariable(UA_Server *server, TaskHandle_t xTask)
{
    UA_ObjectTypeAttributes objTypeAttr = UA_ObjectTypeAttributes_default;

    TaskStatus_t xTaskStatus;
    vTaskGetInfo(xTask, &xTaskStatus, pdTRUE, eInvalid);

    objTypeAttr.displayName = UA_LOCALIZEDTEXT("en-US", "TaskStatsType");

    UA_NodeId taskStatsTypeId = UA_NODEID_STRING(1, "TaskStatsType");
    UA_Server_addObjectTypeNode(
        server,
        taskStatsTypeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "TaskStatsType"),
        objTypeAttr,
        NULL,
        NULL);

    UA_ObjectAttributes objAttr = UA_ObjectAttributes_default;
    objAttr.displayName = UA_LOCALIZEDTEXT("en-US", xTaskStatus.pcTaskName);

    UA_NodeId taskObjId = UA_NODEID_STRING(1, xTaskStatus.pcTaskName);
    UA_NodeId folderId = UA_NODEID_STRING(1, "TaskStats");
    UA_Server_addObjectNode(
        server,
        taskObjId,
        folderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, xTaskStatus.pcTaskName),
        taskStatsTypeId,
        objAttr,
        NULL,
        NULL);

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    char id[50];

    snprintf(id, sizeof(id), "%s.%s", xTaskStatus.pcTaskName, "xTaskNumber");
    UA_Server_addVariableNode(
        server,
        UA_NODEID_STRING(1, id),
        taskObjId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "xTaskNumber"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr,
        NULL,
        NULL);

    snprintf(id, sizeof(id), "%s.%s", xTaskStatus.pcTaskName, "eCurrentState");
    UA_Server_addVariableNode(
        server,
        UA_NODEID_STRING(1, id),
        taskObjId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "eCurrentState"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr,
        NULL,
        NULL);

    snprintf(id, sizeof(id), "%s.%s", xTaskStatus.pcTaskName, "uxCurrentPriority");
    UA_Server_addVariableNode(
        server,
        UA_NODEID_STRING(1, id),
        taskObjId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "uxCurrentPriority"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr,
        NULL,
        NULL);

    snprintf(id, sizeof(id), "%s.%s", xTaskStatus.pcTaskName, "uxBasePriority");
    UA_Server_addVariableNode(
        server,
        UA_NODEID_STRING(1, id),
        taskObjId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "uxBasePriority"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr,
        NULL,
        NULL);

    snprintf(id, sizeof(id), "%s.%s", xTaskStatus.pcTaskName, "ulRunTimeCounter");
    UA_Server_addVariableNode(
        server,
        UA_NODEID_STRING(1, id),
        taskObjId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "ulRunTimeCounter"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr,
        NULL,
        NULL);

    snprintf(id, sizeof(id), "%s.%s", xTaskStatus.pcTaskName, "pxStackBase");
    UA_Server_addVariableNode(
        server,
        UA_NODEID_STRING(1, id),
        taskObjId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "pxStackBase"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr,
        NULL,
        NULL);

    snprintf(id, sizeof(id), "%s.%s", xTaskStatus.pcTaskName, "usStackHighWaterMark");
    UA_Server_addVariableNode(
        server,
        UA_NODEID_STRING(1, id),
        taskObjId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "usStackHighWaterMark"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr,
        NULL,
        NULL);

    updateGetTaskStatsNode(server, xTaskStatus.pcTaskName);

    addValueCallbackToGetTaskStatsVariable(server, xTaskStatus.pcTaskName);
}

static void updateGetTaskStatsNode(UA_Server *server, const char *pcName)
{
    TaskStatus_t xTaskStatus;
    TaskHandle_t xTask = xTaskGetHandle(pcName);
    vTaskGetInfo(xTask, &xTaskStatus, pdTRUE, eInvalid);

    UA_Variant value;
    UA_NodeId nodeId;

    char id[50];

    // xTaskNumber
    UA_Variant xTaskNumbervalue;
    UA_UInt32 xTaskNumber = (UA_UInt32)xTaskStatus.xTaskNumber;
    UA_Variant_setScalar(&xTaskNumbervalue, &xTaskNumber, &UA_TYPES[UA_TYPES_UINT32]);
    snprintf(id, sizeof(id), "%s.%s", xTaskStatus.pcTaskName, "xTaskNumber");
    nodeId = UA_NODEID_STRING(1, id);
    UA_Server_writeValue(server, nodeId, xTaskNumbervalue);

    // xTaskNumber
    UA_Variant eCurrentStatevalue;
    UA_UInt32 eCurrentState = (UA_UInt32)xTaskStatus.eCurrentState;
    UA_Variant_setScalar(&eCurrentStatevalue, &eCurrentState, &UA_TYPES[UA_TYPES_UINT32]);
    snprintf(id, sizeof(id), "%s.%s", xTaskStatus.pcTaskName, "eCurrentState");
    nodeId = UA_NODEID_STRING(1, id);
    UA_Server_writeValue(server, nodeId, eCurrentStatevalue);

    // uxCurrentPriority
    UA_Variant uxCurrentPriorityvalue;
    UA_UInt32 uxCurrentPriority = (UA_UInt32)xTaskStatus.uxCurrentPriority;
    UA_Variant_setScalar(&uxCurrentPriorityvalue, &uxCurrentPriority, &UA_TYPES[UA_TYPES_UINT32]);
    snprintf(id, sizeof(id), "%s.%s", xTaskStatus.pcTaskName, "uxCurrentPriority");
    nodeId = UA_NODEID_STRING(1, id);
    UA_Server_writeValue(server, nodeId, uxCurrentPriorityvalue);

    // uxBasePriority
    UA_Variant uxBasePriorityvalue;
    UA_UInt32 uxBasePriority = (UA_UInt32)xTaskStatus.uxBasePriority;
    UA_Variant_setScalar(&uxBasePriorityvalue, &uxBasePriority, &UA_TYPES[UA_TYPES_UINT32]);
    snprintf(id, sizeof(id), "%s.%s", xTaskStatus.pcTaskName, "uxBasePriority");
    nodeId = UA_NODEID_STRING(1, id);
    UA_Server_writeValue(server, nodeId, uxBasePriorityvalue);

    // ulRunTimeCounter
    UA_Variant ulRunTimeCountervalue;
    UA_UInt32 ulRunTimeCounter = (UA_UInt32)xTaskStatus.ulRunTimeCounter;
    UA_Variant_setScalar(&ulRunTimeCountervalue, &ulRunTimeCounter, &UA_TYPES[UA_TYPES_UINT32]);
    snprintf(id, sizeof(id), "%s.%s", xTaskStatus.pcTaskName, "ulRunTimeCounter");
    nodeId = UA_NODEID_STRING(1, id);
    UA_Server_writeValue(server, nodeId, ulRunTimeCountervalue);

    // pxStackBase
    UA_Variant pxStackBasevalue;
    UA_UInt32 pxStackBase = (UA_UInt32)xTaskStatus.pxStackBase;
    UA_Variant_setScalar(&pxStackBasevalue, &pxStackBase, &UA_TYPES[UA_TYPES_UINT32]);
    snprintf(id, sizeof(id), "%s.%s", xTaskStatus.pcTaskName, "pxStackBase");
    nodeId = UA_NODEID_STRING(1, id);
    UA_Server_writeValue(server, nodeId, pxStackBasevalue);

    // usStackHighWaterMark
    UA_Variant usStackHighWaterMarkvalue;
    UA_UInt32 usStackHighWaterMark = (UA_UInt32)xTaskStatus.usStackHighWaterMark;
    UA_Variant_setScalar(&usStackHighWaterMarkvalue, &usStackHighWaterMark, &UA_TYPES[UA_TYPES_UINT32]);
    snprintf(id, sizeof(id), "%s.%s", xTaskStatus.pcTaskName, "usStackHighWaterMark");
    nodeId = UA_NODEID_STRING(1, id);
    UA_Server_writeValue(server, nodeId, usStackHighWaterMarkvalue);

    UA_Variant_clear(&value);
}

static void
beforeWriteGetTaskStats(UA_Server *server,
                        const UA_NodeId *sessionId, void *sessionContext,
                        const UA_NodeId *nodeid, void *nodeContext,
                        const UA_NumericRange *range, const UA_DataValue *data)
{
    char id[50];
    memcpy(id, nodeid->identifier.string.data, nodeid->identifier.string.length + 1);
    id[nodeid->identifier.string.length] = '\0';
    char *pcName = strtok(id, ".");
    updateGetTaskStatsNode(server, pcName);
}

static void
afterWriteGetTaskStats(UA_Server *server,
                       const UA_NodeId *sessionId, void *sessionContext,
                       const UA_NodeId *nodeId, void *nodeContext,
                       const UA_NumericRange *range, const UA_DataValue *data)
{
}

static void
addValueCallbackToGetTaskStatsVariable(UA_Server *server, const char *pcName)
{
    char id[50];
    snprintf(id, sizeof(id), "%s.%s", pcName, "xTaskNumber");
    UA_NodeId currentNodeId = UA_NODEID_STRING(1, id);

    UA_ValueCallback callback;
    callback.onRead = beforeWriteGetTaskStats;
    callback.onWrite = afterWriteGetTaskStats;
    UA_Server_setVariableNode_valueCallback(server, currentNodeId, callback);
}

#endif // !OPC_FREERTOS_STATUS_H