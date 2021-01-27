import asyncio
from functools import partial

async def conn(txt="hello"):
    # await asyncio.sleep(2)
    print("test")
    await asyncio.sleep(2)
    reader, writer = await asyncio.open_connection(
        '127.0.0.1', 7777)
    await asyncio.sleep(1)
    writer.write(f"{txt}".encode())
    await writer.drain()
    await asyncio.sleep(3)
    writer.close()


async def main(times):
    # loop = asyncio.get_running_loop()
    # for _ in range(times):
    tasks = [conn() for _ in range(times)]
    #     asyncio.create_task(conn())
    await asyncio.gather(*tasks)
        # await loop.create_task(partial(conn)())
    # await asyncio.sleep(1)
    # a = conn("222").send(None)
    # print(a)
    # loop.create_task(conn())
    # loop.run_forever()


if __name__ == '__main__':
    asyncio.run(main(1),debug=True)