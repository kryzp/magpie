#ifndef CORE_METADB
#define CORE_METADB

typedef enum FieldType
{
	FieldType_Int,
	FieldType_Float,
	FieldType_Bool,
	FieldType_String,
	FieldType_Object,
	FieldType_COUNT
}
FieldType;

typedef struct FieldInfo FieldInfo;
struct FieldInfo
{
	String8 name;
	FieldType type;
	u64 offset;
	b32 is_pointer;
};

typedef struct TypeInfo TypeInfo;
struct TypeInfo
{
	TypeInfo *parent;

	String8 name;
	u64 type_id;

	void *(*Factory)(void);

	u32 field_count;
	const FieldInfo *fields;
};

internal inline b32
TypeInfoIsDerivedFrom(const TypeInfo *me, const TypeInfo *other)
{
	const TypeInfo *curr = me;

	while (curr)
	{
		if (curr == other)
			return true;

		curr = curr->parent;
	}

	return false;
}

typedef struct MetaDB MetaDB;
struct MetaDB
{
	// TODO: Hash map [u64 -> TypeInfo]
};

internal void MetaDBBuildRegistry(void);
internal void *MetaDBInstantiate(u64 type_id);

internal const TypeInfo *MetaDBFetch(String8 name);
internal const TypeInfo *MetaDBFetchByID(u64 type_id);

#define MetaDB_DataBegin(type)
#define MetaDB_DataField(name, type)
#define MetaDB_DataEnd()
#define MetaDB_DataEmpty(type)

#endif // CORE_METADB
