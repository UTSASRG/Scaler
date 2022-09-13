import multiprocessing


def do():
    print("Foobar", flush=True)
    raise Exception()
def asdf():

    with multiprocessing.Pool(1) as pool:
        for i in range(5):
            result = pool.apply_async(do)

            result.get()

        pool.close()
        pool.join()

asdf()