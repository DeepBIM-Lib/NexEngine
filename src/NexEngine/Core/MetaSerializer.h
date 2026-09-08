//
// Created by SherSpidey on 2026/6/24.
//

#ifndef NEXENGINE_METASERIALIZER_H
#define NEXENGINE_METASERIALIZER_H

#include <flecs.h>

#include <NexEngine/Core/BaseType.h>

namespace Nex
{
    inline flecs::opaque<String> StringSupport(flecs::world&)
    {
        return flecs::opaque<String>()
               .as_type(flecs::String)
               .serialize([](const flecs::serializer* serializer, const String* data)
               {
                   const char* value = data->c_str();
                   return serializer->value(flecs::String, &value);
               })
               .assign_string([](String* data, const char* value)
               {
                   *data = value;
               });
    }

    template <typename T>
    flecs::opaque<Array<T>, T> ArraySupport(flecs::world& world)
    {
        return flecs::opaque<Array<T>, T>()
               .as_type(world.vector<T>())
               .serialize([](const flecs::serializer* serializer, const Array<T>* data)
               {
                   for (const auto& element : *data)
                   {
                       serializer->value(element);
                   }
                   return 0;
               })
               .count([](const Array<T>* data)
               {
                   return data->size();
               })
               .resize([](Array<T>* data, std::size_t size)
               {
                   data->resize(size);
               })
               .ensure_element([](Array<T>* data, std::size_t element)
               {
                   if (data->size() <= element)
                   {
                       data->resize(element + 1);
                   }
                   return &data->data()[element];
               });
    }


    inline void RegisterMetaString(flecs::world& world)
    {
        world.component<String>().opaque(StringSupport);
    }

    template <typename T>
    void RegisterMetaArray(flecs::world& world)
    {
        world.component<Array<T>>().opaque(ArraySupport<T>);
    }
}

#endif //NEXENGINE_METASERIALIZER_H
