#define MODULE
#define LINUX
#define __KERNEL__

#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */

#include <linux/types.h>

#include <linux/slab.h>
#include <asm/uaccess.h>

#include <linux/fs.h>
#include <linux/miscdevice.h>

#include <linux/string.h>
#include <linux/string_helpers.h>
#include <linux/ctype.h>

#define TYPE_INT 1
#define TYPE_FLOAT 2
#define TYPE_STRING 3
#define TYPE_DICT 4
//#define TYPE_LIST 5 # We don't support lists sorry

#define HASHMAP_SIZE 100

#define STR_COOKIE(s) ((uint32_t*)s)[0]
#define STR_LENGTH(s) ((uint32_t*)s)[1]
#define STR_DATA(s) ((char*)(s+8))
#define STR_MAGIC 0xc001cafe


struct Object {
    int type;
    void* data;
};

struct Node {
    struct Node* next;
    struct Object* key;
    struct Object* value;
};

struct Bucket {
    size_t length;
    struct Bucket* head;
};

struct Hashmap {
    struct Bucket* table[HASHMAP_SIZE];
};

uint64_t hash(struct Object* obj) {
    uint64_t v = 0x0badf00d0badd00d;
    if (obj->type != TYPE_STRING) {
        return (uint64_t) obj->data^v;
    }
    uint32_t strLength = STR_LENGTH(obj->data);

    for (size_t i=0; i<strLength; i++)
        v += (v<<4) + STR_DATA(obj->data)[i];
    return v;
}


void hashmapInit(struct Hashmap* map) {
    for (int i=0; i<HASHMAP_SIZE; i++) {
        map->table[i] = NULL;
    }
}


void hashmapInsert(struct Hashmap* map, struct Object* key, struct Object* value) {
    uint64_t h = hash(key);
    struct Bucket* b = map->table[h%HASHMAP_SIZE];

    struct Node* n = kmalloc(sizeof(struct Node), GFP_KERNEL);
    n->next = NULL;
    n->key = key;
    n->value = value;

    if (b == NULL) {
        b = kmalloc(sizeof(struct Bucket), GFP_KERNEL);
        b->length = 0;
        b->head = NULL;
        map->table[h%HASHMAP_SIZE] = b;
    }

    n->next = b->head;
    b->head = n;

    b->length++;
}

void hashmapMakeCircular(struct Hashmap* map) {
    for (int i=0; i<HASHMAP_SIZE; i++) {
        if (map->table[i] == NULL)
            continue;
        struct Bucket* b = map->table[i];
        struct Node* n = b->head;
        if (n == NULL || n == b)
            continue;

        for (int j=1; j<b->length; j++) {
            n = n->next;
        }

        n->next = b;
    }
}


struct Object* hashmapGet(struct Hashmap* map, struct Object* key) {
    uint64_t h = hash(key);

    struct Bucket* b = map->table[h%HASHMAP_SIZE];
    if (b == NULL)
        return NULL;


    struct Node* n = b->head;
    while(n != NULL && n != b) {
        if (n->key->type != key->type)
            continue;

        if (key->type != TYPE_STRING) {
            if (n->key->data == key->data)
                return n->value;
            continue;
        }

        if (STR_COOKIE(key->data) != STR_MAGIC || STR_COOKIE(n->key->data) != STR_MAGIC)
            continue;

        if (STR_LENGTH(key->data) != STR_LENGTH(n->key->data))
            continue;


        if (memcmp(STR_DATA(key->data), STR_DATA(n->key->data), STR_LENGTH(key->data)))
            continue;

        return n->value;
    }
    return NULL;
}

void hashmapDelete(struct Hashmap* map, struct Object* key) {
    uint64_t h = hash(key);

    struct Bucket* b = map->table[h%HASHMAP_SIZE];
    if (b == NULL)
        return;

    struct Node* n = b->head;
    struct Node* prev = NULL;

    do {
        if (n->key->type != key->type)
            continue;

        if (key->type != TYPE_STRING) {
            if (n->key->data != key->data)
                continue;
        } else {
            if (STR_COOKIE(key->data) != STR_MAGIC || STR_COOKIE(n->key->data) != STR_MAGIC)
                continue;

            if (STR_LENGTH(key->data) != STR_LENGTH(n->key->data))
                continue;

            if (memcmp(STR_DATA(key->data), STR_DATA(n->key->data), STR_LENGTH(key->data)))
                continue;
        }

        if (prev == NULL) {
            b->head = n->next;
        } else {
            prev->next = n->next;
        }

        b->length--;
        kfree(n);
        return;

    } while((prev = n) != NULL && (n = n->next) != NULL && n != b);
}



struct Object* deserializeString(void* ptr) {
    uint32_t length;
    if (get_user(length, (uint32_t*)(ptr+0x10)))
        return NULL;

    if (length > 0x1000)
        return NULL;

    size_t mallocSize = length+sizeof(uint32_t)*2+1;
    void* string = kmalloc(mallocSize, GFP_KERNEL);
    if (string == NULL)
        return NULL;

    memset(string, 0, mallocSize);

    STR_COOKIE(string) = STR_MAGIC;
    STR_LENGTH(string) = length;
    if (copy_from_user(STR_DATA(string), ptr+0x24, length)) {
        kfree(string);
        return NULL;
    }
    struct Object* obj = kmalloc(sizeof(struct Object), GFP_KERNEL);
    obj->data = string;
    return obj;
}


struct Object* deserialize(void* ptr);
struct Object* deserializePrimative(void* ptr);

size_t objCount = 0;
void** objTracker = NULL;

struct Object* deserializeDict(void* ptr) {
    uint32_t count;
    if (get_user(count, (uint32_t*)(ptr+0x20)))
        return NULL;
    count++;

    struct Hashmap* map = kmalloc(sizeof(struct Hashmap), GFP_KERNEL);
    hashmapInit(map);

    void* mapTable;
    if (get_user(mapTable, (void**)(ptr+0x28)))
        return NULL;

    for (int i=0; i<count; i++) {
        uint64_t key;
        uint64_t value;
        if (get_user(key, (uint64_t*)(mapTable+i*0x18+0x8)))
            return NULL;
        if (get_user(value, (uint64_t*)(mapTable+i*0x18+0x10)))
            return NULL;
        if (key == NULL || value == NULL)
            continue;


        struct Object* keyObj = deserializePrimative(key);
        if (keyObj == NULL)
            continue;

        struct Object* valueObj = deserialize(value);
        if (valueObj == NULL)
            continue;

        hashmapInsert(map, keyObj, valueObj);
    }

    struct Object* obj = kmalloc(sizeof(struct Object), GFP_KERNEL);
    obj->type = TYPE_DICT;
    obj->data = map;
    return obj;
}

int getType(void* ptr) {
    char name[10];
    char* typePtr;

    memset(name, 0, 10);

    if (get_user(typePtr, (char**)(ptr+8)))
        return 0;

    if (get_user(typePtr, (char**)(typePtr+0x18)))
        return 0;

    if (copy_from_user(name, typePtr, 10))
        return 0;

    if (!strcmp(name, "int"))
        return TYPE_INT;

    if (!strcmp(name, "float"))
        return TYPE_FLOAT;

    if (!strcmp(name, "str"))
        return TYPE_STRING;

    if (!strcmp(name, "dict"))
        return TYPE_DICT;
    return 0;
}

void trackObject(void* ptr, struct Object* obj) {
    objTracker[objCount*2] = ptr;
    objTracker[objCount*2+1] = obj;
    objCount++;
}

struct Object* deserializePrimative(void* ptr) {
    int type = getType(ptr);
    if (!type)
        return NULL;


    struct Object* obj;
    if (type == TYPE_INT || type == TYPE_FLOAT) {

        uint64_t value;
        if (get_user(value, (uint64_t*)(ptr+0x10)))
            return NULL;
        obj = kmalloc(sizeof(struct Object), GFP_KERNEL);
        obj->data = value;
    } else {
        obj = deserializeString(ptr);
    }
    obj->type = type;

    trackObject(ptr, obj);

    return obj;
}

struct Object* deserialize(void* ptr) {
    int type = getType(ptr);
    if (!type)
        return NULL;


    struct Object* obj = NULL;

    if (type == TYPE_DICT) {
        // Check if we used this pointer yet.
        // If we did don't do it again to prevent circular structure
        for (int i=0; i<objCount; i++) {
            if (objTracker[i*2] == ptr)
                return NULL;
        }

        obj = deserializeDict(ptr);

        trackObject(ptr, obj);
        return obj;
    } else if (type == TYPE_STRING || type == TYPE_INT || type == TYPE_FLOAT) {
        return deserializePrimative(ptr);
    } else {
        return NULL;
        printk("Unknown..\n");
    }
}

struct Object* deserializeRoot(void* ptr) {
    objTracker = kmalloc(sizeof(void*) * 200, GFP_KERNEL);
    memset(objTracker, 0, sizeof(void*) * 200);
    objCount = 0;

    struct Object* root = deserialize(ptr);

    if (root == NULL)
        return NULL;


    // Fixup hashmaps
    for (int i=0; i<objCount; i++) {
        struct Object* obj = objTracker[i*2+1];
        if (obj == NULL)
            continue;
        if (obj->type == TYPE_DICT)
            hashmapMakeCircular(obj->data);
    }

    kfree(objTracker);

    return root;
}

static struct Hashmap* mainMap;

// Turn an object into a json blob
int dumpJson(struct Object* obj, char* outstr, size_t length) {
    size_t used = 0;
    if (obj->type == TYPE_INT) {
        char str[32];
        snprintf(str, 32, "%lu", obj->data);
        size_t addedLength = strlen(str);
        if (used + addedLength >= length)
            return 0;
        used += addedLength;
        strcat(outstr, str);
        return used;
    }

    if (obj->type == TYPE_FLOAT) {
        char str[32];
        snprintf(str, 32, "%lf", obj->data);
        size_t addedLength = strlen(str);
        if (used + addedLength >= length)
            return 0;
        used += addedLength;
        strcat(outstr, str);
        return used;
    }

    if (obj->type == TYPE_STRING) {
        char* ptr = STR_DATA(obj->data);

        if (STR_COOKIE(obj->data) != STR_MAGIC)
            return 0;

        if (used == length)
            return 0;
        outstr[used++] = '"';

        for (int i=0; i<STR_LENGTH(obj->data); i++) {
            if (used == length)
                return 0;

            // Escape some special json characters
            if (ptr[i] == '"') {
                if (used >= length-1)
                    return 0;
                outstr[used++] = '\\';
                outstr[used++] = '"';
            } else if (ptr[i] == '\n') {
                if (used >= length-1)
                    return 0;
                outstr[used++] = '\\';
                outstr[used++] = 'n';
            } else if (ptr[i] == '\r') {
                if (used >= length-1)
                    return 0;
                outstr[used++] = '\\';
                outstr[used++] = 'r';
            } else if (ptr[i] == '\\') {
                if (used >= length-1)
                    return 0;
                outstr[used++] = '\\';
                outstr[used++] = '\\';

            // Catch all for any remaining unprintable characters (Techincally not up to spec)
            } else if ((ptr[i]!=' ' && isspace(ptr[i])) || !isprint(ptr[i])) {
                if (used >= length-3)
                    return 0;
                snprintf(&outstr[used], 5, "\\x%02x", ptr[i]);
                used+=4;

            } else {
                outstr[used++] = ptr[i];
            }
        }

        if (used == length)
            return 0;
        outstr[used++] = '"';

        return used;
    }

    if (obj->type == TYPE_DICT) {
        if (used == length)
            return 0;
        outstr[used++] = '{';

        struct Hashmap* map = obj->data;
        for (int i=0; i<HASHMAP_SIZE; i++) {
            if (map->table[i] == NULL)
                continue;
            struct Bucket* b = map->table[i];
            struct Node* n = b->head;
            if (n == NULL || n == b)
                continue;


            for (int j=0; j<b->length; j++) {
                size_t subUsed = dumpJson(n->key, &outstr[used], length-used);
                if (!subUsed)
                    return 0;
                used += subUsed;

                if (used >= length-1)
                    return 0;
                outstr[used++] = ':';
                outstr[used++] = ' ';

                subUsed = dumpJson(n->value, &outstr[used], length-used);
                if (!subUsed)
                    return 0;
                used += subUsed;

                n = n->next;
            }
        }

        if (used == length)
            return 0;
        outstr[used++] = '}';
        return used;
    }
    return 0;
}





static int handleIoctl(struct file *f, unsigned int cmd, void** arg) {
    printk("Got IOCTL %x\n",cmd);
    if (cmd == 1) {
        void* key_ptr;
        void* value_ptr;
        if (get_user(key_ptr, arg) || get_user(value_ptr, &arg[1]))
            return -EINVAL;
        struct Object* key = deserializeRoot(key_ptr);
        if (key == NULL)
            return -EINVAL;

        struct Object* value = deserializeRoot(value_ptr);
        if (value == NULL)
            return -EINVAL;

        hashmapInsert(mainMap, key, value);
        printk("Successfully created object\n");
        return 0;
    }
    
    if (cmd == 4) {
        void* buffer;
        size_t length;
        if (get_user(buffer, arg) || get_user(length, (size_t*)(&arg[1])))
            return -EINVAL;
        if (!access_ok(VERIFY_WRITE, buffer, length))
            return -EINVAL;

        void* key_ptr;
        if (get_user(key_ptr, &arg[2]))
            return -EINVAL;
        struct Object* key = deserializeRoot(key_ptr);
        if (key == NULL)
            return -EINVAL;

        memset(buffer, 0, length);

        struct Object* value = hashmapGet(mainMap, key);
        if (value == NULL)
            return 0;

        int res = dumpJson(value, buffer, length);

        if (res == 0)
            return -EINVAL;
        printk("Successfully dumped object %s\n",buffer);
        return 0;
    }

    if (cmd == 5) {
        void* key_ptr;
        if (get_user(key_ptr, arg))
            return -EINVAL;
        struct Object* key = deserializeRoot(key_ptr);
        if (key == NULL)
            return -EINVAL;
        hashmapDelete(mainMap, key);
        printk("Successfully deleted object\n");
        return 0;
    }
    return EINVAL;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = handleIoctl
};




static struct miscdevice dev = {
    MISC_DYNAMIC_MINOR, "kws" , &fops
};


int init_module(void) {
    printk("Hello world\n");

    mainMap = kmalloc(sizeof(struct Hashmap), GFP_KERNEL);
    hashmapInit(mainMap);

    int ret = misc_register(&dev); 

    return 0;
}

void cleanup_module(void) {
    misc_deregister(&dev);
    printk(KERN_ALERT "Goodbye\n");
}

MODULE_LICENSE("GPL");  
