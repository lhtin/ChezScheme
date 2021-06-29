# The Format of Boot File

## 编码方式介绍

- 按字节解析
- usigned long int编码：
  - 有效字节结束于最后一位为0的字节，如果某个字节最后一位为1，表示后面一个字节也包含在内
  - 因此每个字节包含的数字内容为前7位，并且前面的字节为高有效位
  - 比如 0x49 15 0A
    - 0x49和0x15的最后一位为1，0A的最后一位为0，所以总共包含三个字节
    - 值的计算为：(0x49 >> 1 << 7 << 7) + (0x15 >> 1 << 7) + (0x0A >> 1) = 591109 = 0x90505

## 内容

- magic
  - 前4个字节为0，后四个字节为chez的ASCII编码
- scheme_version（usigned long int编码）
  - 0x90505 => 9.5.5
- machine_type（usigned long int编码）
  - D表示x86_64
- required boot files
  - 比如scheme.boot依赖了petite.boot，因此scheme.boot中该位置为`(petite)`
  - petite.boot不依赖任何其他boot file，所以为`()`