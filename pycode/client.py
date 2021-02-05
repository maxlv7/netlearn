import asyncio


async def tcp_echo_client(message):
    await asyncio.sleep(3)
    reader, writer = await asyncio.open_connection(
        '127.0.0.1', 7777)
    # await asyncio.sleep(3)
    print(f'Send: {message!r}')
    writer.write(message.encode())

    data = await reader.read(100)
    print(f'send:{message!r} -->Received: {data.decode()!r}')
    assert message == data.decode()

    print('Close the connection')
    writer.close()


async def main(times):
    tasks2 = [tcp_echo_client(str(f"message: {_}")) for _ in range(times)]
    await asyncio.gather(*tasks2)


if __name__ == '__main__':
    for x in range(1000):
        asyncio.run(main(100), debug=True)
