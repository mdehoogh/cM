nl::json custom_interpreter::execute_request_impl(int execution_counter, // Typically the cell number
                                                  const std::string& /*code*/, // Code to execute
                                                  bool /*silent*/,
                                                  bool /*store_history*/,
                                                  nl::json /*user_expressions*/,
                                                  bool /*allow_stdin*/)
{
    // You can use the C-API of your target language for executing the code,
    // e.g. `PyRun_String` for the Python C-API
    //      `luaL_dostring` for the Lua C-API

    // Use this method for publishing the execution result to the client,
    // this method takes the ``execution_counter`` as first argument,
    // the data to publish (mime type data) as second argument and metadata
    // as third argument.
    // Replace "Hello World !!" by what you want to be displayed under the execution cell
    nl::json pub_data;
    pub_data["text/plain"] = "Hello World !!";
    publish_execution_result(execution_counter, std::move(pub_data), nl::json::object());

    // You can also use this method for publishing errors to the client, if the code
    // failed to execute
    // publish_execution_error(error_name, error_value, error_traceback);
    publish_execution_error("TypeError", "123", {"!@#$", "*(*"});

    return xeus::create_successful_reply();
}

void custom_interpreter::configure_impl()
